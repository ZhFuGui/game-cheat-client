#include "volume.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace mousebox::games::cs2::hitboxes
{
namespace
{
Vec3 add(Vec3 a, Vec3 b) noexcept
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
Vec3 sub(Vec3 a, Vec3 b) noexcept
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
Vec3 scale(Vec3 a, double value) noexcept
{
    return {a.x * value, a.y * value, a.z * value};
}
Vec3 cross(Vec3 a, Vec3 b) noexcept
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
double dot(Vec3 a, Vec3 b) noexcept
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
double length(Vec3 point) noexcept
{
    return std::hypot(point.x, point.y, point.z);
}
bool finite(Vec3 point) noexcept
{
    return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
}

double remapDepth(double depth, PointDistribution distribution) noexcept
{
    depth = std::clamp(depth, 0.0, 1.0);
    switch (distribution)
    {
    case PointDistribution::Center:
        return depth * depth * depth;
    case PointDistribution::Outer:
        return std::sqrt(depth);
    case PointDistribution::Middle:
        // The volume inside a similar, centered shape scales with the cube of depth.
        return std::cbrt(std::lerp(.4 * .4 * .4, .8 * .8 * .8, depth * depth * depth));
    default:
        return depth;
    }
}
Vec3 moveAlongRay(Vec3 center, Vec3 offset, double depth, PointDistribution distribution) noexcept
{
    return depth > 0 ? add(center, scale(offset, remapDepth(depth, distribution) / depth)) : center;
}
std::optional<Vec3> boxPoint(const Definition &box, PointDistribution distribution,
                             const std::array<double, 5> &u) noexcept
{
    const Vec3 half{std::abs(box.b.x - box.a.x) * .5, std::abs(box.b.y - box.a.y) * .5,
                    std::abs(box.b.z - box.a.z) * .5};
    const auto center = scale(add(box.a, box.b), .5);
    if (!finite(center) || !finite(half) || half.x <= 0 || half.y <= 0 || half.z <= 0)
        return {};
    const Vec3 unit{2 * u[0] - 1, 2 * u[1] - 1, 2 * u[2] - 1};
    const Vec3 offset{unit.x * half.x, unit.y * half.y, unit.z * half.z};
    const auto depth = std::max({std::abs(unit.x), std::abs(unit.y), std::abs(unit.z)});
    if (distribution == PointDistribution::Uniform)
        return add(center, offset);
    if (depth == 0 && distribution == PointDistribution::Middle)
        return add(center, {half.x * remapDepth(0, distribution), 0, 0});
    return moveAlongRay(center, offset, depth, distribution);
}
double capsuleDepth(Vec3 offset, Vec3 axis, double halfLength, double radius) noexcept
{
    const double along = std::abs(dot(offset, axis));
    const double radial = length(cross(offset, axis));
    const double wallDepth = radial / radius;
    if (along <= halfLength * wallDepth)
        return wallDepth;
    // This ray exits through an end hemisphere. Solve its distance from the midpoint.
    const auto distance = std::hypot(along, radial);
    const auto perpendicular = halfLength * (radial / distance);
    const auto reach = halfLength * (along / distance) +
                       radius * std::sqrt(std::max(0.0, 1 - (perpendicular / radius) * (perpendicular / radius)));
    return distance / reach;
}
std::optional<Vec3> capsulePoint(const Definition &shape, PointDistribution distribution,
                                 const std::array<double, 5> &u) noexcept
{
    const auto segment = shape.shape == Shape::Sphere ? Vec3{} : sub(shape.b, shape.a);
    const auto span = length(segment);
    const auto radius = shape.radius;
    if (!std::isfinite(span) || !std::isfinite(radius) || radius <= 0)
        return {};
    const auto center = add(shape.a, scale(segment, .5));
    if (!finite(center))
        return {};
    const auto axis = span > 0 ? scale(segment, 1 / span) : Vec3{0, 0, 1};
    auto side = cross(axis, std::abs(axis.z) < .8 ? Vec3{0, 0, 1} : Vec3{0, 1, 0});
    side = scale(side, 1 / length(side));
    const auto angle = 2 * std::numbers::pi * u[3];
    const auto circle = add(scale(side, std::cos(angle)), scale(cross(axis, side), std::sin(angle)));
    const auto half = span * .5;
    const auto cylinderShare = span / (span + 4.0 / 3 * radius);
    Vec3 offset;
    if (u[0] < cylinderShare)
    {
        offset = add(scale(axis, (2 * u[1] - 1) * half), scale(circle, radius * std::sqrt(u[2])));
    }
    else
    {
        const auto distance = radius * std::cbrt(u[2]);
        const auto axial = half + distance * u[4];
        const auto radial = distance * std::sqrt(1 - u[4] * u[4]);
        offset = add(scale(axis, u[1] < .5 ? axial : -axial), scale(circle, radial));
    }
    if (distribution == PointDistribution::Uniform)
        return add(center, offset);
    return moveAlongRay(center, offset, capsuleDepth(offset, axis, half, radius), distribution);
}
} // namespace

Definition insetDefinition(Definition shape, double fraction) noexcept
{
    if (fraction && shape.shape == Shape::Box)
    {
        const auto center = scale(add(shape.a, shape.b), .5);
        shape.a = add(center, scale(sub(shape.a, center), 1 - fraction));
        shape.b = add(center, scale(sub(shape.b, center), 1 - fraction));
    }
    else
        shape.radius *= 1 - fraction;
    return shape;
}
std::optional<Vec3> sampleInterior(const Definition &shape, PointDistribution distribution,
                                   const std::array<double, 5> &units) noexcept
{
    if (!finite(shape.a) || (shape.shape != Shape::Sphere && !finite(shape.b)) ||
        std::any_of(units.begin(), units.end(), [](double u) { return !(u > 0 && u < 1); }))
        return {};
    if (shape.shape == Shape::Box)
        return boxPoint(shape, distribution, units);
    if (shape.shape == Shape::Capsule || shape.shape == Shape::Sphere)
        return capsulePoint(shape, distribution, units);
    return {};
}
} // namespace mousebox::games::cs2::hitboxes
