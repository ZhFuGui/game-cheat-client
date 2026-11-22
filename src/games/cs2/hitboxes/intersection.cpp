#include "intersection.hpp"
#include "volume.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace mousebox::games::cs2::hitboxes
{
namespace
{
Vec3 sub(Vec3 a, Vec3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
Vec3 add(Vec3 a, Vec3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
Vec3 mul(Vec3 a, double n)
{
    return {a.x * n, a.y * n, a.z * n};
}
double dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec3 cross(Vec3 a, Vec3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
bool finite(Vec3 p)
{
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
std::optional<double> capsule(Vec3 o, Vec3 d, Vec3 a, Vec3 b, double radius)
{
    if (!(radius > 0) || !std::isfinite(radius))
        return {};
    const auto axis = sub(b, a), q = sub(o, a);
    const auto aa = dot(axis, axis), qa = dot(q, axis), da = dot(d, axis);
    std::vector<double> boundaries{0, 1};
    if (aa > 1e-20 && std::abs(da) > 1e-20)
        for (const auto u : {0., aa})
        {
            const auto t = (u - qa) / da;
            if (t > 0 && t < 1)
                boundaries.push_back(t);
        }
    std::sort(boundaries.begin(), boundaries.end());
    for (std::size_t i = 1; i < boundaries.size(); ++i)
    {
        const auto lo = boundaries[i - 1], hi = boundaries[i];
        auto v = q, w = d;
        const auto projection = qa + (lo + hi) * .5 * da;
        if (aa > 1e-20 && projection >= aa)
            v = sub(o, b);
        else if (aa > 1e-20 && projection > 0)
        {
            v = sub(q, mul(axis, qa / aa));
            w = sub(d, mul(axis, da / aa));
        }
        const auto A = dot(w, w), B = 2 * dot(v, w), C = dot(v, v) - radius * radius;
        if ((A * lo + B) * lo + C <= 0)
            return lo;
        if (A <= 1e-24)
            continue;
        const auto discriminant = B * B - 4 * A * C;
        if (discriminant < 0)
            continue;
        const auto t = (-B - std::sqrt(discriminant)) / (2 * A);
        if (t >= lo && t <= hi)
            return t;
    }
    return {};
}
std::optional<double> box(Vec3 o, Vec3 d, Vec3 a, Vec3 b)
{
    double lo = 0, hi = 1;
    const double origins[]{o.x, o.y, o.z}, deltas[]{d.x, d.y, d.z}, lower[]{a.x, a.y, a.z}, upper[]{b.x, b.y, b.z};
    for (unsigned i = 0; i < 3; ++i)
    {
        const auto low = std::min(lower[i], upper[i]), high = std::max(lower[i], upper[i]);
        if (std::abs(deltas[i]) < 1e-20)
        {
            if (origins[i] < low || origins[i] > high)
                return {};
            continue;
        }
        auto x = (low - origins[i]) / deltas[i], y = (high - origins[i]) / deltas[i];
        if (x > y)
            std::swap(x, y);
        lo = std::max(lo, x);
        hi = std::min(hi, y);
        if (lo > hi)
            return {};
    }
    return lo;
}
} // namespace
Vec3 TargetIntersector::Entry::inverse(Vec3 v) const
{
    return {dot(v, yz) / determinant, dot(v, zx) / determinant, dot(v, xy) / determinant};
}
TargetIntersector::TargetIntersector(const Pose &pose)
{
    if (!pose.model || pose.missing || pose.invalid || pose.hitboxes.empty() ||
        pose.hitboxes.size() != pose.model->definitions.size())
        return;
    entries_.reserve(pose.hitboxes.size());
    for (const auto &hitbox : pose.hitboxes)
    {
        if (!hitbox.definition)
            return;
        const auto origin = hitbox.point({0, 0, 0});
        const auto x = sub(hitbox.point({1, 0, 0}), origin), y = sub(hitbox.point({0, 1, 0}), origin),
                   z = sub(hitbox.point({0, 0, 1}), origin);
        const auto determinant = dot(x, cross(y, z));
        if (!std::isfinite(determinant) || std::abs(determinant) < 1e-20)
            return;
        entries_.push_back({hitbox.definition, origin, cross(y, z), cross(z, x), cross(x, y), determinant});
    }
    valid_ = true;
}
std::optional<Vec3> TargetIntersector::localPoint(unsigned id, Vec3 world) const
{
    if (valid_ && finite(world))
        for (const auto &entry : entries_)
            if (entry.definition->id == id)
                return entry.inverse(sub(world, entry.origin));
    return {};
}
std::optional<Intersection> TargetIntersector::first(Vec3 start, Vec3 end, double insetFraction) const
{
    if (!valid_ || !finite(start) || !finite(end) || !std::isfinite(insetFraction) || insetFraction < 0 ||
        insetFraction >= 1)
        return {};
    const auto direction = sub(end, start);
    if (dot(direction, direction) < 1e-20)
        return {};
    std::optional<Intersection> result;
    bool ambiguous = false;
    for (const auto &entry : entries_)
    {
        const auto local = entry.inverse(sub(start, entry.origin)), delta = entry.inverse(direction);
        const auto d = insetDefinition(*entry.definition, insetFraction);
        const auto t = d.shape == Shape::Box
                           ? box(local, delta, d.a, d.b)
                           : capsule(local, delta, d.a, d.shape == Shape::Sphere ? d.a : d.b, d.radius);
        if (!t)
            continue;
        if (!result || *t < result->fraction - 1e-9)
        {
            result = Intersection{add(start, mul(direction, *t)), d.group, d.id, *t};
            ambiguous = false;
        }
        else if (std::abs(*t - result->fraction) <= 1e-9 && d.group != result->group)
            ambiguous = true;
    }
    if (ambiguous || (result && result->fraction <= 0))
        return {};
    return result;
}
std::optional<Intersection> firstIntersection(const Pose &pose, Vec3 start, Vec3 end)
{
    return TargetIntersector(pose).first(start, end);
}
} // namespace mousebox::games::cs2::hitboxes
