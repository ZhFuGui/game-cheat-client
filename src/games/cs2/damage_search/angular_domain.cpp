#include "angular_domain.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace mousebox::games::cs2::damage_search
{
namespace
{
Vec3 sub(Vec3 a, Vec3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
double dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec3 cross(Vec3 a, Vec3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
Vec3 unit(Vec3 v)
{
    const auto n = std::sqrt(dot(v, v));
    return {v.x / n, v.y / n, v.z / n};
}
Vec3 center(const hitboxes::PosedHitbox &box)
{
    const auto &d = *box.definition;
    return box.point(d.shape == hitboxes::Shape::Sphere
                         ? d.a
                         : Vec3{(d.a.x + d.b.x) * .5, (d.a.y + d.b.y) * .5, (d.a.z + d.b.z) * .5});
}
std::array<Vec3, 8> corners(const hitboxes::PosedHitbox &box)
{
    const auto &d = *box.definition;
    const auto b = d.shape == hitboxes::Shape::Sphere ? d.a : d.b;
    const auto r = d.shape == hitboxes::Shape::Box ? 0 : d.radius;
    const Vec3 lo{std::min(d.a.x, b.x) - r, std::min(d.a.y, b.y) - r, std::min(d.a.z, b.z) - r};
    const Vec3 hi{std::max(d.a.x, b.x) + r, std::max(d.a.y, b.y) + r, std::max(d.a.z, b.z) + r};
    std::array<Vec3, 8> result;
    for (unsigned i = 0; i < 8; ++i)
        result[i] = box.point({i & 1 ? hi.x : lo.x, i & 2 ? hi.y : lo.y, i & 4 ? hi.z : lo.z});
    return result;
}
} // namespace
AngularPoint AngularRect::center() const
{
    return {(left + right) * .5, (bottom + top) * .5};
}
bool AngularRect::overlaps(const AngularRect &b) const
{
    return left <= b.right && right >= b.left && bottom <= b.top && top >= b.bottom;
}
std::array<AngularRect, 4> AngularRect::split() const
{
    const auto m = center();
    return {{{left, bottom, m.u, m.v}, {m.u, bottom, right, m.v}, {left, m.v, m.u, top}, {m.u, m.v, right, top}}};
}
std::optional<AngularPoint> AngularDomain::project(Vec3 p) const
{
    const auto v = sub(p, origin);
    const auto depth = dot(v, forward);
    if (!std::isfinite(depth) || depth <= 1e-6)
        return {};
    AngularPoint result{dot(v, right) / depth, dot(v, up) / depth};
    if (!std::isfinite(result.u) || !std::isfinite(result.v))
        return {};
    return result;
}
Vec3 AngularDomain::endpoint(AngularPoint p) const
{
    const auto d = unit({forward.x + right.x * p.u + up.x * p.v, forward.y + right.y * p.u + up.y * p.v,
                         forward.z + right.z * p.u + up.z * p.v});
    return {origin.x + d.x * rayLength, origin.y + d.y * rayLength, origin.z + d.z * rayLength};
}
std::optional<AngularDomain> angularDomain(const hitboxes::Pose &pose, Vec3 origin)
{
    if (!pose.model || pose.missing || pose.invalid || pose.hitboxes.empty() ||
        pose.hitboxes.size() != pose.model->definitions.size())
        return {};
    Vec3 mean{};
    for (const auto &box : pose.hitboxes)
    {
        const auto p = center(box);
        mean.x += p.x;
        mean.y += p.y;
        mean.z += p.z;
    }
    const double count = static_cast<double>(pose.hitboxes.size());
    mean = {mean.x / count, mean.y / count, mean.z / count};
    const auto direction = sub(mean, origin);
    if (!std::isfinite(dot(direction, direction)) || dot(direction, direction) < 1e-12)
        return {};
    AngularDomain result;
    result.origin = origin;
    result.forward = unit(direction);
    result.right = unit(cross(result.forward, std::abs(result.forward.z) < .95 ? Vec3{0, 0, 1} : Vec3{0, 1, 0}));
    result.up = cross(result.right, result.forward);
    for (const auto &box : pose.hitboxes)
    {
        Footprint footprint;
        footprint.id = box.definition->id;
        footprint.group = box.definition->group;
        footprint.bounds = {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(),
                            -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};
        for (const auto p : corners(box))
        {
            const auto projected = result.project(p);
            if (!projected)
                return {}; // Cannot bound a silhouette crossing the projection plane.
            auto &b = footprint.bounds;
            b.left = std::min(b.left, projected->u);
            b.right = std::max(b.right, projected->u);
            b.bottom = std::min(b.bottom, projected->v);
            b.top = std::max(b.top, projected->v);
            const auto v = sub(p, origin);
            result.rayLength = std::max(result.rayLength, std::sqrt(dot(v, v)));
        }
        footprint.representative = result.project(center(box)).value_or(footprint.bounds.center());
        // Outward floating point rounding keeps tangent silhouette edges in the domain.
        auto &b = footprint.bounds;
        b.left = std::nextafter(b.left, -INFINITY);
        b.bottom = std::nextafter(b.bottom, -INFINITY);
        b.right = std::nextafter(b.right, INFINITY);
        b.top = std::nextafter(b.top, INFINITY);
        result.footprints.push_back(footprint);
    }
    result.rayLength = std::nextafter(result.rayLength + 1, INFINITY);
    return result;
}
} // namespace mousebox::games::cs2::damage_search
