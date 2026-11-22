#include "observations.hpp"
#include <cmath>

namespace mousebox::scene
{
namespace
{
double dot(Vec3 a, Vec3 b) noexcept
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
} // namespace
bool validBasis(Vec3 a, Vec3 b, Vec3 c) noexcept
{
    constexpr double tolerance = 1e-6;
    return domain::finite(a) && domain::finite(b) && domain::finite(c) && std::abs(dot(a, a) - 1) < tolerance &&
           std::abs(dot(b, b) - 1) < tolerance && std::abs(dot(c, c) - 1) < tolerance &&
           std::abs(dot(a, b)) < tolerance && std::abs(dot(a, c)) < tolerance && std::abs(dot(b, c)) < tolerance;
}
std::optional<Vec3> toMetric(Vec3 value, const MetricTransform &transform) noexcept
{
    const auto &a = transform.nativeAxes;
    if (!transform.verified || !transform.metersPerUnit || !std::isfinite(*transform.metersPerUnit) ||
        *transform.metersPerUnit <= 0 || !domain::finite(value) || !domain::finite(transform.originMeters) ||
        !validBasis(a[0], a[1], a[2]))
        return {};
    const auto scale = *transform.metersPerUnit;
    const auto origin = transform.originMeters;
    const Vec3 result{origin.x + scale * (a[0].x * value.x + a[1].x * value.y + a[2].x * value.z),
                      origin.y + scale * (a[0].y * value.x + a[1].y * value.y + a[2].y * value.z),
                      origin.z + scale * (a[0].z * value.x + a[1].z * value.y + a[2].z * value.z)};
    return domain::finite(result) ? std::optional(result) : std::nullopt;
}
std::optional<Vec3> cameraSpace(const CameraPose &camera, const Position &point) noexcept
{
    if (point.spaceId.empty() || point.spaceId != camera.position.spaceId ||
        point.originEpoch != camera.position.originEpoch || !domain::finite(point.value) ||
        !domain::finite(camera.position.value) || !validBasis(camera.forward, camera.right, camera.up))
        return {};
    const Vec3 delta{point.value.x - camera.position.value.x, point.value.y - camera.position.value.y,
                     point.value.z - camera.position.value.z};
    const Vec3 result{dot(delta, camera.right), dot(delta, camera.up), dot(delta, camera.forward)};
    return domain::finite(result) ? std::optional(result) : std::nullopt;
}
} // namespace mousebox::scene
