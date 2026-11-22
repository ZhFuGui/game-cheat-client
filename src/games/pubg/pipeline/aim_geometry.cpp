#include "games/pubg/model/control_frame.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace mousebox::control
{
bool validCalibration(const Calibration &c) noexcept
{
    return (c.measured != c.formulaEstimated) && !c.profile.empty() && std::isfinite(c.yawDegreesPerCount) &&
           std::isfinite(c.pitchDegreesPerCount) && std::abs(c.yawDegreesPerCount) > 1e-9 &&
           std::abs(c.pitchDegreesPerCount) > 1e-9 && std::isfinite(c.fov) && c.fov > 0 && c.fov < 180;
}
std::optional<AngularError> angleTo(const Frame &view, Vec3 p, Vec3 origin)
{
    if (!view.camera || !view.cameraOrigin || !view.rotation || !finite(p) || !finite(origin) ||
        !finite(*view.camera) || !finite(*view.cameraOrigin) || !finite(*view.rotation))
        return {};
    const auto cam = *view.camera, co = *view.cameraOrigin, r = *view.rotation;
    const Vec3 d{(p.x + origin.x) - (cam.x + co.x), (p.y + origin.y) - (cam.y + co.y),
                 (p.z + origin.z) - (cam.z + co.z)};
    const double distance = std::hypot(d.x, d.y, d.z);
    if (!std::isfinite(distance) || distance < 1e-6)
        return {};
    constexpr double radians = std::numbers::pi / 180, degrees = 180 / std::numbers::pi;
    const double sp = std::sin(r.x * radians), cp = std::cos(r.x * radians);
    const double sy = std::sin(r.y * radians), cy = std::cos(r.y * radians);
    const double sr = std::sin(r.z * radians), cr = std::cos(r.z * radians);
    const Vec3 f{cp * cy, cp * sy, sp}, right{sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp};
    const Vec3 up{-cr * sp * cy - sr * sy, -cr * sp * sy + sr * cy, cr * cp};
    const auto dot = [&](Vec3 axis) { return d.x * axis.x + d.y * axis.y + d.z * axis.z; };
    const double forward = dot(f), horizontal = dot(right), vertical = dot(up);
    return AngularError{std::atan2(horizontal, forward) * degrees,
                        std::atan2(vertical, std::hypot(forward, horizontal)) * degrees,
                        std::acos(std::clamp(forward / distance, -1.0, 1.0)) * degrees, distance};
}
} // namespace mousebox::control
