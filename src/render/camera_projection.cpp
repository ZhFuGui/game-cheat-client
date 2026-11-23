#include "camera_projection.hpp"
#include "domain/scene/observations.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace mousebox::render
{
using domain::Vec3;
namespace
{
double dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
} // namespace
std::optional<CameraProjection> CameraProjection::create(CameraBasis camera, EspRect rect, double fov, EspFovAxis axis,
                                                         std::optional<double> ortho)
{
    if (!domain::finite(camera.position) || !scene::validBasis(camera.forward, camera.right, camera.up) ||
        !std::isfinite(rect.x) || !std::isfinite(rect.y) || !std::isfinite(rect.width) || !std::isfinite(rect.height) ||
        rect.width <= 0 || rect.height <= 0)
        return {};
    if (ortho ? !std::isfinite(*ortho) || *ortho <= 0 : !std::isfinite(fov) || fov <= 0 || fov >= 180)
        return {};
    CameraProjection p;
    p.camera_ = camera;
    p.rect_ = rect;
    p.perspective_ = !ortho;
    p.scale_ = ortho ? rect.width / *ortho
                     : (axis == EspFovAxis::Horizontal ? rect.width : rect.height) /
                           (2 * std::tan(fov * std::numbers::pi / 360));
    if (!std::isfinite(p.scale_) || p.scale_ <= 0)
        return {};
    return p;
}
Vec3 CameraProjection::local(Vec3 world) const
{
    const Vec3 d{world.x - camera_.position.x, world.y - camera_.position.y, world.z - camera_.position.z};
    return {dot(d, camera_.right), dot(d, camera_.up), dot(d, camera_.forward)};
}
std::optional<Point> CameraProjection::screen(Vec3 p) const
{
    if (!domain::finite(p) || p.z <= 0)
        return {};
    const auto factor = perspective_ ? scale_ / p.z : scale_;
    const auto x = rect_.x + rect_.width / 2. + p.x * factor, y = rect_.y + rect_.height / 2. - p.y * factor;
    if (!std::isfinite(x) || !std::isfinite(y) || std::abs(x) > std::numeric_limits<float>::max() ||
        std::abs(y) > std::numeric_limits<float>::max())
        return {};
    return Point{static_cast<float>(x), static_cast<float>(y)};
}
std::optional<Point> CameraProjection::point(Vec3 world) const
{
    return screen(local(world));
}
std::optional<Line> CameraProjection::segment(Vec3 a, Vec3 b) const
{
    const auto first = local(a), last = local(b);
    if (!domain::finite(first) || !domain::finite(last))
        return {};
    const auto near =
        std::max(std::numeric_limits<double>::denorm_min(),
                 std::numeric_limits<double>::epsilon() * std::max({1., std::abs(first.z), std::abs(last.z)}));
    const auto planes = [&](Vec3 p) {
        const auto depth = perspective_ ? p.z : 1.;
        return std::array{p.z - near, rect_.width / 2. * depth + scale_ * p.x, rect_.width / 2. * depth - scale_ * p.x,
                          rect_.height / 2. * depth + scale_ * p.y, rect_.height / 2. * depth - scale_ * p.y};
    };
    const auto from = planes(first), to = planes(last);
    double enter = 0, leave = 1;
    for (unsigned i = 0; i < from.size(); ++i)
    {
        if (!std::isfinite(from[i]) || !std::isfinite(to[i]) || (from[i] < 0 && to[i] < 0))
            return {};
        if (from[i] < 0 || to[i] < 0)
        {
            const auto t = from[i] / (from[i] - to[i]);
            if (from[i] < 0)
                enter = std::max(enter, t);
            else
                leave = std::min(leave, t);
            if (enter > leave)
                return {};
        }
    }
    const auto at = [&](double t) {
        auto p = screen({std::lerp(first.x, last.x, t), std::lerp(first.y, last.y, t), std::lerp(first.z, last.z, t)});
        if (p)
        {
            p->x = std::clamp(p->x, rect_.x, rect_.x + rect_.width);
            p->y = std::clamp(p->y, rect_.y, rect_.y + rect_.height);
        }
        return p;
    };
    const auto p = at(enter), q = at(leave);
    return p && q ? std::optional(Line{*p, *q}) : std::nullopt;
}
} // namespace mousebox::render
