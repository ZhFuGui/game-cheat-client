#include "sampling.hpp"
#include <cmath>
#include <exception>

namespace mousebox::games::cs2::extensions
{
namespace
{
bool finitePoint(domain::Vec3 p)
{
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
domain::Vec3 between(domain::Vec3 a, domain::Vec3 b, double t)
{
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}
} // namespace
Visibility RayBudget::query(domain::Vec3 target)
{
    if (!remaining_ || stop_.stop_requested() || !finitePoint(origin_) || !finitePoint(target) ||
        std::hypot(target.x - origin_.x, target.y - origin_.y, target.z - origin_.z) <= 1e-6)
    {
        ++unknown;
        return Visibility::Unknown;
    }
    --remaining_;
    ++queries;
    Visibility result = Visibility::Unknown;
    try
    {
        result = scene_.query(origin_, target);
    }
    catch (const std::exception &)
    {
    }
    if (result == Visibility::Unknown)
        ++unknown;
    return result;
}
std::vector<Span> sampleBone(RayBudget &rays, domain::Vec3 a, domain::Vec3 b, Visibility atA, Visibility atB)
{
    constexpr unsigned subdivisions = 16, refinements = 6;
    std::vector<Span> result;
    const auto append = [&](double from, double to, Visibility visibility) {
        if (to <= from)
            return;
        if (!result.empty() && result.back().visibility == visibility && result.back().end == from)
            result.back().end = to;
        else
            result.push_back({from, to, visibility});
    };
    double left = 0;
    auto state = atA;
    for (unsigned i = 1; i <= subdivisions; ++i)
    {
        const double right = static_cast<double>(i) / subdivisions;
        auto next = i == subdivisions ? atB : rays.query(between(a, b, right));
        if (next == state)
            append(left, right, state);
        else if (state == Visibility::Unknown || next == Visibility::Unknown)
            append(left, right, Visibility::Unknown);
        else
        {
            double lo = left, hi = right;
            bool resolved = true;
            for (unsigned n = 0; n < refinements; ++n)
            {
                const double middle = (lo + hi) / 2;
                const auto visible = rays.query(between(a, b, middle));
                if (visible == Visibility::Unknown)
                {
                    resolved = false;
                    break;
                }
                if (visible == state)
                    lo = middle;
                else
                    hi = middle;
            }
            if (!resolved)
                append(left, right, Visibility::Unknown);
            else
            {
                // Keep the unresolved boundary interval neutral. Never extend
                // a positive classification across a failed/unknown ray.
                append(left, lo, state);
                append(lo, hi, Visibility::Unknown);
                append(hi, right, next);
            }
        }
        left = right;
        state = next;
    }
    return result;
}
} // namespace mousebox::games::cs2::extensions
