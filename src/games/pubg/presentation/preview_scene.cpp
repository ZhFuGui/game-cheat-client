#include "preview_scene.hpp"
#include "esp_scene.hpp"
#include <cmath>
#include <numbers>
#include <unordered_map>

namespace mousebox::render
{
Primitives SceneProjector::project(const Scene &scene, float width, float height)
{
    Primitives result;
    if (scene.esp)
    {
        if (!esp_)
            esp_ = std::make_shared<EspProjector>();
        result.esp = esp_->project(scene, width, height);
        return result;
    }
    result.title = scene.english ? L"View & skeleton preview" : L"视点与骨骼预览";
    if (width <= 0 || height <= 0 || !std::isfinite(width) || !std::isfinite(height))
        return result;
    if (previous_ != scene.living)
    {
        previous_ = scene.living;
        links_.clear();
        if (previous_)
            for (const auto &character : previous_->characters)
            {
                std::unordered_map<unsigned, std::size_t> indices;
                indices.reserve(character.bones.size());
                for (std::size_t i = 0; i < character.bones.size(); ++i)
                    indices.emplace(character.bones[i].index, i);
                auto &links = links_.emplace_back();
                links.reserve(character.bones.size());
                for (std::size_t i = 0; i < character.bones.size(); ++i)
                {
                    const auto parent = character.bones[i].parent;
                    if (parent && *parent >= 0)
                        if (const auto found = indices.find(static_cast<unsigned>(*parent)); found != indices.end())
                            links.emplace_back(i, found->second);
                }
            }
    }
    const auto &v = scene.view;
    if (!scene.living || !v || !v->fov || !v->camera || !v->cameraOrigin || !v->rotation || !std::isfinite(*v->fov) ||
        *v->fov <= 0 || *v->fov >= 180 || !control::finite(*v->camera) || !control::finite(*v->cameraOrigin) ||
        !control::finite(*v->rotation))
    {
        result.message = scene.english ? L"Connect Reader or load a recording to view current data"
                                       : L"连接 Reader 或载入录制样本，查看当前数据";
        return result;
    }
    constexpr double rad = std::numbers::pi / 180;
    const auto r = *v->rotation;
    const double sp = std::sin(r.x * rad), cp = std::cos(r.x * rad), sy = std::sin(r.y * rad), cy = std::cos(r.y * rad);
    const double sr = std::sin(r.z * rad), cr = std::cos(r.z * rad), focal = width / 2.0 / std::tan(*v->fov * rad / 2);
    const control::Vec3 forward{cp * cy, cp * sy, sp}, right{sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp};
    const control::Vec3 up{-cr * sp * cy - sr * sy, -cr * sp * sy + sr * cy, cr * cp};
    const auto camera = *v->camera, origin = *v->cameraOrigin;
    const auto project = [&](control::Vec3 point, control::Vec3 pointOrigin) -> std::optional<Point> {
        if (!control::finite(point) || !control::finite(pointOrigin))
            return {};
        const control::Vec3 d{(point.x + pointOrigin.x) - (camera.x + origin.x),
                              (point.y + pointOrigin.y) - (camera.y + origin.y),
                              (point.z + pointOrigin.z) - (camera.z + origin.z)};
        const auto dot = [&](control::Vec3 axis) { return d.x * axis.x + d.y * axis.y + d.z * axis.z; };
        const double depth = dot(forward), distance = std::hypot(d.x, d.y, d.z);
        if (!std::isfinite(distance) || distance < 1e-6 || depth <= distance * std::cos(89 * rad))
            return {};
        const double x = width / 2.0 + dot(right) / depth * focal, y = height / 2.0 - dot(up) / depth * focal;
        if (!std::isfinite(x) || !std::isfinite(y) || std::abs(x) > 1e7 || std::abs(y) > 1e7)
            return {};
        return Point{static_cast<float>(x), static_cast<float>(y)};
    };
    const auto inside = [&](Point p) { return p.x >= 0 && p.x < width && p.y >= 0 && p.y < height; };
    for (std::size_t ci = 0; ci < scene.living->characters.size(); ++ci)
    {
        const auto &c = scene.living->characters[ci];
        if (!scene.showSelf && scene.living->self &&
            (c.identity == *scene.living->self || control::sameNativeAddress(c.identity, *scene.living->self)))
            continue;
        const unsigned group = scene.target && c.identity == scene.target->identity ? 1 : 0;
        std::vector<std::optional<Point>> points;
        points.reserve(c.bones.size());
        for (const auto &bone : c.bones)
        {
            const auto point = project(bone.position, bone.origin);
            points.push_back(point);
            if (point && inside(*point))
            {
                result.dots[group].push_back(*point);
                ++result.visiblePoints;
            }
        }
        for (const auto &[a, b] : links_[ci])
            if (points[a] && points[b])
                result.skeletons[group].push_back({*points[a], *points[b]});
        if (c.head && c.headOrigin)
            if (const auto head = project(*c.head, *c.headOrigin); head && inside(*head))
                result.heads[group].push_back(*head);
        if (group && scene.target->position && scene.target->origin)
            result.aim = project(*scene.target->position, *scene.target->origin);
    }
    if (!result.visiblePoints)
        result.message =
            !scene.showSelf && scene.living->characters.size() == 1 && scene.living->self
                ? (scene.english ? L"This sample contains only self, excluded from target selection"
                                 : L"当前样本仅包含自己 · 已排除目标选择")
                : (scene.english ? L"No drawable bones in the current view" : L"当前视野内没有可显示的骨点");
    return result;
}
} // namespace mousebox::render
