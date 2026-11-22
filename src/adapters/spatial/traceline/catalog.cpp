#include "catalog.hpp"
#include "platform/windows/dynamic_library.hpp"
#include <cs2-external-autowall/types.h>
#include <nlohmann/json.hpp>
#include <charconv>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>

namespace mousebox::adapters::traceline
{
namespace
{
using Json = nlohmann::json;
void require(bool valid, const char *message)
{
    if (!valid)
        throw std::runtime_error(message);
}
std::string text(const Json &value, const char *key, std::size_t maximum = 1024)
{
    const auto &field = value.at(key);
    require(field.is_string(), "掩体场景清单文本无效");
    auto result = field.get<std::string>();
    require(!result.empty() && result.size() <= maximum && result.find('\0') == std::string::npos,
            "掩体场景清单文本无效");
    return result;
}
std::uint64_t number(const Json &value, const char *key, std::uint64_t maximum)
{
    const auto &field = value.at(key);
    require(field.is_number_unsigned() || (field.is_number_integer() && field.get<std::int64_t>() >= 0),
            "掩体场景清单数值无效");
    const auto result = field.get<std::uint64_t>();
    require(result <= maximum, "掩体场景清单数值越界");
    return result;
}
FileIdentity file(const Json &value, const char *extension)
{
    FileIdentity out{text(value, "file"), text(value, "sha256", 64), number(value, "bytes", UINT64_MAX)};
    require(out.sha256.size() == 64 && out.sha256.find_first_not_of("0123456789abcdef") == std::string::npos &&
                out.bytes > 0,
            "掩体场景清单摘要无效");
    const std::filesystem::path relative(out.file);
    require(out.file.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_./-") ==
                    std::string::npos &&
                !relative.is_absolute() && !relative.has_root_name() && relative.extension() == extension,
            "掩体场景文件路径无效");
    for (const auto &part : relative)
        require(part != ".." && part != "." && !part.empty(), "掩体场景文件路径越界");
    return out;
}
} // namespace
Catalog::Catalog(const std::filesystem::path &root)
try : root_(std::filesystem::canonical(root))
{
    const auto path = root_ / "manifest.json";
    require(std::filesystem::file_size(path) <= 4 * 1024 * 1024, "掩体场景清单过大");
    std::ifstream stream(path, std::ios::binary);
    require(static_cast<bool>(stream), "未找到掩体场景清单");
    const auto json = Json::parse(stream);
    require(number(json, "schema", 1) == 1 && text(json, "scope") == "static-physics" &&
                text(json, "breakables") == "intact",
            "掩体场景清单版本不兼容");
    abi = static_cast<unsigned>(number(json, "abi", UINT32_MAX));
    library = file(json.at("library"), ".dll");
    if (json.contains("ballistics"))
    {
        ballistics = file(json.at("ballistics"), CS2TL_PROFILES_EXTENSION);
        ballisticsGameBuild = static_cast<unsigned>(number(json.at("ballistics"), "gameBuild", UINT32_MAX));
        require(ballisticsGameBuild != 0 && ballistics->file != library.file, "穿透参数包版本无效");
    }
    const auto &entries = json.at("maps");
    require(entries.is_array() && !entries.empty() && entries.size() <= 4096, "掩体地图目录无效");
    std::set<std::string> names, resources, files{library.file};
    if (ballistics)
        files.insert(ballistics->file);
    for (const auto &entry : entries)
    {
        MapEntry map{text(entry, "name", 256), text(entry, "resource")};
        require(names.insert(map.name).second && resources.insert(map.resource).second, "掩体地图身份重复");
        const auto &scenes = entry.at("scenes");
        require(scenes.is_array() && !scenes.empty() && scenes.size() <= 2, "掩体场景模式无效");
        std::set<std::string> variants;
        for (const auto &value : scenes)
        {
            SceneEntry scene;
            static_cast<FileIdentity &>(scene) = file(value, CS2TL_SCENE_CACHE_EXTENSION);
            scene.variant = text(value, "variant");
            require((scene.variant == "intact" || scene.variant == "ignore-breakables") &&
                        variants.insert(scene.variant).second && files.insert(scene.file).second,
                    "掩体场景模式重复或不支持");
            const auto fingerprint = text(value, "fingerprint", 20);
            const auto parsed =
                std::from_chars(fingerprint.data(), fingerprint.data() + fingerprint.size(), scene.fingerprint);
            require(parsed.ec == std::errc{} && parsed.ptr == fingerprint.data() + fingerprint.size(),
                    "掩体场景指纹无效");
            scene.triangles = static_cast<std::uint32_t>(number(value, "triangles", UINT32_MAX));
            scene.solids = static_cast<std::uint32_t>(number(value, "solids", UINT32_MAX));
            scene.closedSolids = static_cast<std::uint32_t>(number(value, "closedSolids", UINT32_MAX));
            scene.nodes = static_cast<std::uint32_t>(number(value, "nodes", UINT32_MAX));
            scene.flags = static_cast<std::uint32_t>(number(value, "flags", 2));
            require(!(scene.flags & 1) && scene.closedSolids <= scene.solids, "掩体场景几何声明无效");
            map.scenes.push_back(std::move(scene));
        }
        require(variants.contains("intact"), "掩体地图缺少完整静态场景");
        maps.push_back(std::move(map));
    }
}
catch (const Json::exception &)
{
    throw std::runtime_error("静态掩体场景清单无效");
}
const SceneEntry &Catalog::scene(std::string_view map, std::string_view resource) const
{
    for (const auto &entry : maps)
        if (entry.name == map && entry.resource == resource)
            for (const auto &scene : entry.scenes)
                if (scene.variant == "intact")
                    return scene;
    throw std::runtime_error("当前地图尚未包含在静态掩体资源包中");
}
std::filesystem::path Catalog::verify(const FileIdentity &entry) const
{
    const auto path = std::filesystem::canonical(root_ / entry.file);
    const auto relative = path.lexically_relative(root_);
    require(!relative.empty() && !relative.is_absolute(), "掩体场景文件路径越界");
    for (const auto &part : relative)
        require(part != "..", "掩体场景文件路径越界");
    require(std::filesystem::is_regular_file(path) && std::filesystem::file_size(path) == entry.bytes &&
                platform::fileSha256(path) == entry.sha256,
            "掩体资源文件损坏或版本不匹配");
    return path;
}
} // namespace mousebox::adapters::traceline
