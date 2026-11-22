#include "intersection.hpp"
#include <algorithm>
#include <array>
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
Vec3 mul(Vec3 a, double s)
{
    return {a.x * s, a.y * s, a.z * s};
}
double dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
// Distance between a forward ray and a finite capsule axis, in exact local space.
double capsuleRatio(Vec3 o, Vec3 d, Vec3 a, Vec3 b, double radius)
{
    if (!(radius > 0))
        return std::numeric_limits<double>::infinity();
    const auto axis = sub(b, a), q = sub(o, a);
    const double A = dot(d, d), B = dot(d, axis), C = dot(axis, axis), D = dot(d, q), E = dot(axis, q);
    double best = std::numeric_limits<double>::infinity();
    const auto consider = [&](double u) {
        u = std::clamp(u, 0., 1.);
        const auto v = sub(q, mul(axis, u));
        const double t = -dot(d, v) / A;
        if (t <= 0)
            return;
        const auto delta = add(v, mul(d, t));
        best = std::min(best, std::sqrt(dot(delta, delta)) / radius);
    };
    consider(0);
    consider(1);
    const double determinant = A * C - B * B;
    if (determinant > 1e-20)
        consider((A * E - B * D) / determinant);
    if (C > 1e-20)
        consider(E / C);
    return best;
}
// Minimum centered box expansion along a forward ray. Its L-infinity norm is
// convex piecewise linear; all finite minima occur at a pair of six faces.
double boxRatio(Vec3 origin, Vec3 direction, Vec3 a, Vec3 b)
{
    const auto c = mul(add(a, b), .5), h = mul(sub(b, a), .5), q = sub(origin, c);
    const std::array<double, 3> half{std::abs(h.x), std::abs(h.y), std::abs(h.z)}, o{q.x, q.y, q.z},
        d{direction.x, direction.y, direction.z};
    for (auto v : half)
        if (v <= 1e-12)
            return std::numeric_limits<double>::infinity();
    std::array<double, 6> base{}, slope{};
    for (unsigned i = 0; i < 3; ++i)
    {
        base[2 * i] = o[i] / half[i];
        base[2 * i + 1] = -base[2 * i];
        slope[2 * i] = d[i] / half[i];
        slope[2 * i + 1] = -slope[2 * i];
    }
    double best = std::numeric_limits<double>::infinity();
    for (unsigned i = 0; i < 6; ++i)
        for (unsigned j = i + 1; j < 6; ++j)
        {
            const auto denominator = slope[i] - slope[j];
            if (std::abs(denominator) < 1e-20)
                continue;
            const auto t = (base[j] - base[i]) / denominator;
            if (t <= 0)
                continue;
            double ratio = 0;
            for (unsigned k = 0; k < 6; ++k)
                ratio = std::max(ratio, base[k] + slope[k] * t);
            best = std::min(best, ratio);
        }
    return best;
}
} // namespace
std::optional<double> TargetIntersector::directionProximity(Vec3 start, Vec3 end) const
{
    if (!valid_)
        return {};
    const auto worldDirection = sub(end, start);
    if (!std::isfinite(dot(worldDirection, worldDirection)) || dot(worldDirection, worldDirection) <= 1e-20)
        return {};
    if (first(start, end))
        return 1.;
    double best = 0;
    for (const auto &entry : entries_)
    {
        const auto o = entry.inverse(sub(start, entry.origin)), d = entry.inverse(worldDirection);
        const auto &shape = *entry.definition;
        const auto center = shape.shape == Shape::Sphere ? shape.a : mul(add(shape.a, shape.b), .5);
        if (dot(sub(center, o), d) <= 0)
            continue;
        const auto ratio =
            shape.shape == Shape::Box
                ? boxRatio(o, d, shape.a, shape.b)
                : capsuleRatio(o, d, shape.a, shape.shape == Shape::Sphere ? shape.a : shape.b, shape.radius);
        if (std::isfinite(ratio))
            best = std::max(best, std::min(.999999, 1 / std::max(1., ratio)));
    }
    return best;
}
} // namespace mousebox::games::cs2::hitboxes
