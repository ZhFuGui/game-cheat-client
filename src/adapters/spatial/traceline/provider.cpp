#include "provider.hpp"
#include "catalog.hpp"
#include "ballistics.hpp"
#include "platform/windows/dynamic_library.hpp"
#include <cs2-external-autowall/api.h>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <map>
#include <mutex>

namespace mousebox::adapters::traceline
{
namespace
{
struct Api
{
    platform::DynamicLibrary library;
    decltype(&cs2tl_load_scene) load;
    decltype(&cs2tl_destroy_scene) destroy;
    decltype(&cs2tl_get_scene_info) info;
    decltype(&cs2tl_is_occluded) occluded;
    explicit Api(const std::filesystem::path &path, unsigned abi)
        : library(path), load(reinterpret_cast<decltype(load)>(library.symbol("cs2tl_load_scene"))),
          destroy(reinterpret_cast<decltype(destroy)>(library.symbol("cs2tl_destroy_scene"))),
          info(reinterpret_cast<decltype(info)>(library.symbol("cs2tl_get_scene_info"))),
          occluded(reinterpret_cast<decltype(occluded)>(library.symbol("cs2tl_is_occluded")))
    {
        const auto version =
            reinterpret_cast<decltype(&cs2tl_get_abi_version)>(library.symbol("cs2tl_get_abi_version"));
        if (abi != CS2TL_ABI_VERSION || version() != abi)
            throw std::runtime_error("掩体检测运行库ABI版本不兼容");
    }
};
class Scene final : public application::ports::VisibilityScene
{
    std::shared_ptr<const Api> api_;
    cs2tl_scene *scene_ = nullptr;

  public:
    Scene(std::shared_ptr<const Api> api, const std::filesystem::path &path, const SceneEntry &entry)
        : api_(std::move(api))
    {
        const auto bytes = path.u8string();
        const std::string utf8(bytes.begin(), bytes.end());
        cs2tl_error error{sizeof(cs2tl_error)};
        const auto code = api_->load(utf8.c_str(), &scene_, &error);
        if (code != CS2TL_OK || !scene_)
        {
            if (scene_)
                api_->destroy(std::exchange(scene_, nullptr));
            throw std::runtime_error("静态掩体场景无法加载，请检查资源包");
        }
        cs2tl_scene_info info{sizeof(cs2tl_scene_info)};
        if (api_->info(scene_, &info, &error) != CS2TL_OK || info.source_fingerprint != entry.fingerprint ||
            info.triangle_count != entry.triangles || info.solid_count != entry.solids ||
            info.closed_solid_count != entry.closedSolids || info.bvh_node_count != entry.nodes ||
            info.flags != entry.flags || (info.flags & CS2TL_SCENE_APPROXIMATE))
        {
            api_->destroy(std::exchange(scene_, nullptr));
            throw std::runtime_error("静态掩体场景与地图清单不匹配");
        }
    }
    ~Scene() override
    {
        api_->destroy(scene_);
    }
    cs2tl_scene *native() const noexcept
    {
        return scene_;
    }
    application::ports::Visibility query(domain::Vec3 origin, domain::Vec3 target) const override
    {
        using application::ports::Visibility;
        const auto valid = [](domain::Vec3 p) {
            constexpr auto maximum = std::numeric_limits<float>::max();
            return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) && std::abs(p.x) <= maximum &&
                   std::abs(p.y) <= maximum && std::abs(p.z) <= maximum;
        };
        if (!valid(origin) || !valid(target))
            return Visibility::Unknown;
        const cs2tl_segment segment{
            sizeof(cs2tl_segment),
            0,
            {static_cast<float>(origin.x), static_cast<float>(origin.y), static_cast<float>(origin.z)},
            {static_cast<float>(target.x), static_cast<float>(target.y), static_cast<float>(target.z)},
            0,
            0};
        std::uint32_t result = 0;
        if (api_->occluded(scene_, &segment, &result, nullptr) != CS2TL_OK || result > 1)
            return Visibility::Unknown;
        return result ? Visibility::Occluded : Visibility::Visible;
    }
};
} // namespace
struct Provider::Impl
{
    std::filesystem::path root;
    std::optional<Catalog> catalog;
    std::shared_ptr<const Api> api;
    std::shared_ptr<const detail::ProfilePackage> profiles;
    std::map<std::string, std::weak_ptr<const Scene>> scenes;
    std::mutex mutex;
    explicit Impl(std::filesystem::path path) : root(std::move(path)) {}
    std::shared_ptr<const Scene> scene(std::string_view map, std::string_view resource)
    {
        if (!catalog)
            catalog.emplace(root);
        const auto &entry = catalog->scene(map, resource);
        if (auto existing = scenes[entry.file].lock())
            return existing;
        if (!api)
            api = std::make_shared<const Api>(catalog->verify(catalog->library), catalog->abi);
        auto loaded = std::make_shared<const Scene>(api, catalog->verify(entry), entry);
        scenes[entry.file] = loaded;
        return loaded;
    }
};
Provider::Provider(std::filesystem::path root) : impl_(std::make_unique<Impl>(std::move(root))) {}
Provider::~Provider() = default;
bool Provider::available() const noexcept
{
    return true;
}
std::shared_ptr<const application::ports::VisibilityScene> Provider::load(std::string_view map,
                                                                          std::string_view resource)
{
    try
    {
        std::lock_guard lock(impl_->mutex);
        return impl_->scene(map, resource);
    }
    catch (const std::filesystem::filesystem_error &)
    {
        throw std::runtime_error("静态掩体资源缺失或不可读取，请检查安装包");
    }
}
std::shared_ptr<const application::ports::BallisticsScene> Provider::loadBallistics(std::string_view map,
                                                                                    std::string_view resource,
                                                                                    unsigned /*gameBuild*/)
{
    try
    {
        std::lock_guard lock(impl_->mutex);
        auto scene = impl_->scene(map, resource);
        if (!impl_->catalog->ballistics)
            throw std::runtime_error("缺少匹配规则的穿透参数包");
        if (!impl_->profiles)
            impl_->profiles =
                detail::loadProfiles(std::shared_ptr<const platform::DynamicLibrary>(impl_->api, &impl_->api->library),
                                     impl_->catalog->verify(*impl_->catalog->ballistics));
        const auto handle = scene->native();
        return detail::bindBallistics(impl_->profiles, handle, std::move(scene));
    }
    catch (const std::filesystem::filesystem_error &)
    {
        throw std::runtime_error("穿透参数或静态场景资源缺失，请检查安装包");
    }
}
} // namespace mousebox::adapters::traceline
