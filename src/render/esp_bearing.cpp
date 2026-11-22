#include "esp_bearing.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace mousebox::render
{
BearingRing bearingLayout(float width, float height)
{
    BearingRing ring;
    if (!std::isfinite(width) || !std::isfinite(height) || width < 160 || height < 160)
        return ring;
    ring.radius = std::min(58.f, std::min(width, height) * .16f);
    const auto inset = ring.radius + 26;
    ring.center = {inset, height - inset};
    return ring;
}
void appendBearing(BearingRing &ring, domain::Vec3 camera, domain::Vec3 person, double yaw, unsigned group,
                   std::uint64_t id)
{
    if (!ring.ready || ring.radius <= 0 || !domain::finite(camera) || !domain::finite(person) || !std::isfinite(yaw))
        return;
    const double x = person.x - camera.x, y = person.y - camera.y, length = std::hypot(x, y);
    if (!std::isfinite(length) || length <= 0)
        return;
    const double radians = std::remainder(yaw, 360.) * std::numbers::pi / 180.;
    const double cy = std::cos(radians), sy = std::sin(radians);
    const double nx = x / length, ny = y / length;
    ring.markers.push_back({{ring.center.x + ring.radius * static_cast<float>(-nx * sy + ny * cy),
                             ring.center.y - ring.radius * static_cast<float>(nx * cy + ny * sy)},
                            group,
                            id});
}
} // namespace mousebox::render
