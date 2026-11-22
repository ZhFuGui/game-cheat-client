#include "control/ballistics.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <numbers>

namespace mousebox::control
{
namespace
{
Vec3 add(Vec3 a, Vec3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
Vec3 sub(Vec3 a, Vec3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
Vec3 mul(Vec3 a, double b)
{
    return {a.x * b, a.y * b, a.z * b};
}
double dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec3 cross(Vec3 a, Vec3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
double norm(Vec3 a)
{
    return std::hypot(a.x, a.y, a.z);
}
Vec3 unit(Vec3 a)
{
    return mul(a, 1 / norm(a));
}
double pitch(Vec3 a)
{
    return std::atan2(a.z, std::hypot(a.x, a.y)) * 180 / std::numbers::pi;
}
Vec3 turn(Vec3 value, Vec3 from, Vec3 to)
{
    const auto k = cross(from, to);
    const auto c = std::clamp(dot(from, to), -1.0, 1.0);
    return add(add(value, cross(k, value)), mul(cross(k, cross(k, value)), 1 / (1 + c)));
}
struct Intercept
{
    double time = 0, travel = 0, residual = 0;
    Vec3 direction, impact, target;
};
std::optional<Intercept> intercept(const FlightTimeCurve &curve, Vec3 origin, Vec3 target, Vec3 velocity,
                                   double gravity, double maximum)
{
    const Vec3 r = sub(target, origin), g{0, 0, -gravity};
    const auto desired = [&](double t) { return sub(add(r, mul(velocity, t)), mul(g, 0.5 * t * t)); };
    const auto f = [&](double s) {
        const auto t = curve.timeAt(s);
        return t ? norm(desired(*t)) - s : INFINITY;
    };
    double previous = 0, lo = 0, hi = 0;
    bool bracket = false;
    // No game lifetime is invented: 10s/10km are bounded numerical search limits.
    const auto maximumTime = curve.timeAt(maximum);
    if (!maximumTime)
        return {};
    if (*maximumTime > 10)
    {
        double a = 0, b = maximum;
        for (unsigned i = 0; i < 52; ++i)
        {
            const auto m = (a + b) / 2;
            if (*curve.timeAt(m) > 10)
                b = m;
            else
                a = m;
        }
        maximum = a;
    }
    for (unsigned i = 1; i <= 192; ++i)
    {
        const double x = static_cast<double>(i) / 192, s = maximum * x * x;
        if (f(s) <= 0)
        {
            lo = previous;
            hi = s;
            bracket = true;
            break;
        }
        previous = s;
    }
    if (!bracket)
        return {};
    for (unsigned i = 0; i < 64 && hi - lo > 1e-7; ++i)
    {
        const auto m = (lo + hi) / 2;
        if (f(m) > 0)
            lo = m;
        else
            hi = m;
    }
    Intercept result;
    result.travel = (lo + hi) / 2;
    result.time = *curve.timeAt(result.travel);
    const auto d = desired(result.time);
    if (norm(d) < 1e-9)
        return {};
    result.direction = unit(d);
    result.target = add(target, mul(velocity, result.time));
    result.impact = add(add(origin, mul(result.direction, result.travel)), mul(g, 0.5 * result.time * result.time));
    result.residual = norm(sub(result.impact, result.target));
    return result;
}
} // namespace
FlightTimeCurve::FlightTimeCurve(const BallisticProfile &p) : initial_(p.initialSpeed)
{
    if (!std::isfinite(initial_) || initial_ <= 0)
        return;
    if (p.velocityKeys.size() < 2)
    {
        valid_ = true;
        return;
    }
    double last = -INFINITY;
    for (const auto &[x, v] : p.velocityKeys)
    {
        if (!std::isfinite(x) || !std::isfinite(v) || v <= 0 || x <= last)
            return;
        last = x;
    }
    if (p.velocityKeys.front().first > 0)
        return;
    // Negative published abscissae may bracket x=0. They inform interpolation
    // only; flight time always starts at zero distance, never a negative launch.
    double zero = p.velocityKeys.back().second;
    for (std::size_t i = 1; i < p.velocityKeys.size(); ++i)
        if (p.velocityKeys[i].first >= 0)
        {
            const auto [a, va] = p.velocityKeys[i - 1];
            const auto [b, vb] = p.velocityKeys[i];
            zero = va + (vb - va) * (0 - a) / (b - a);
            break;
        }
    keys_.push_back({0, zero});
    times_.push_back(0);
    for (const auto &key : p.velocityKeys)
        if (key.first > 0)
        {
            const auto [a, va] = keys_.back();
            const auto [b, vb] = key;
            times_.push_back(times_.back() + (b - a) * (1 / va + 1 / vb) / 2);
            keys_.push_back(key);
        }
    valid_ = curve_ = true;
}
std::optional<double> FlightTimeCurve::timeAt(double distance) const noexcept
{
    if (!valid_ || !std::isfinite(distance) || distance < 0)
        return {};
    if (!curve_)
        return distance / initial_;
    const auto end =
        std::upper_bound(keys_.begin(), keys_.end(), distance, [](double x, const auto &key) { return x < key.first; });
    if (end == keys_.end())
        return times_.back() + (distance - keys_.back().first) / keys_.back().second;
    const auto index = static_cast<std::size_t>(end - keys_.begin() - 1);
    const auto [a, va] = keys_[index];
    const auto [b, vb] = *end;
    const auto vd = va + (vb - va) * (distance - a) / (b - a);
    return times_[index] + (distance - a) * (1 / va + 1 / vd) / 2;
}
BallisticSolution solveBallistic(const BallisticProfile &profile, const BallisticRequest &request)
{
    const FlightTimeCurve curve(profile);
    return solveBallistic(profile, curve, request);
}
BallisticSolution solveBallistic(const BallisticProfile &profile, const FlightTimeCurve &curve,
                                 const BallisticRequest &request) noexcept
{
    BallisticSolution result;
    if (!finite(request.origin) || !finite(request.camera) || !finite(request.target) ||
        !finite(request.targetVelocity) || !finite(request.sightDirection) || norm(request.sightDirection) < 1e-6 ||
        norm(sub(request.target, request.origin)) < 0.01 || !validGravity(request.gravity) ||
        !std::isfinite(request.zeroingMeters) || request.zeroingMeters <= 0)
        return result;
    result.usedCurve = curve.usesCurve();
    const double maximum = profile.maximumTravel.value_or(10000);
    if (!curve.valid() || !std::isfinite(maximum) || maximum <= 0 || maximum > 100000)
    {
        result.status = BallisticStatus::InvalidProfile;
        return result;
    }
    const auto shot =
        intercept(curve, request.origin, request.target, request.targetVelocity, request.gravity, maximum);
    if (!shot)
    {
        result.status = BallisticStatus::NoIntercept;
        return result;
    }
    result.flightSeconds = shot->time;
    result.travelMeters = shot->travel;
    result.launchDirection = result.aimDirection = shot->direction;
    result.dropMeters = request.gravity * shot->time * shot->time / 2;
    result.impactPoint = shot->impact;
    result.targetAtImpact = shot->target;
    result.residualMeters = shot->residual;
    if (result.residualMeters > 0.002)
    {
        result.status = BallisticStatus::ResidualRejected;
        return result;
    }
    if (request.zeroing == ZeroingRule::EquivalentAngle)
    {
        const auto sight = unit(request.sightDirection);
        const auto reference = intercept(curve, request.origin, add(request.camera, mul(sight, request.zeroingMeters)),
                                         {}, request.gravity, maximum);
        if (!reference || dot(reference->direction, sight) < -0.999999)
        {
            result.status = BallisticStatus::ZeroingUnreachable;
            return result;
        }
        result.zeroingSeconds = reference->time;
        result.zeroingPitchDegrees = pitch(reference->direction) - pitch(sight);
        result.zeroingDirection = reference->direction;
        // Remove the reference zero rotation, preserving yaw/roll and high-low
        // geometry. Live requests use the same aligned camera as both origins.
        result.aimDirection = unit(turn(result.launchDirection, reference->direction, sight));
    }
    result.status = BallisticStatus::Ready;
    return result;
}
ProfileMatch BallisticCatalog::find(const std::string &actorClass, const std::string &itemId) const noexcept
{
    if (actorClass.empty() && itemId.empty())
        return {};
    const auto contains = [](const auto &values, const auto &value) {
        return std::any_of(values.begin(), values.end(), [&](const auto &key) {
            return key.size() == value.size() &&
                   std::equal(key.begin(), key.end(), value.begin(),
                              [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); });
        });
    };
    const CompiledBallisticProfile *found = nullptr;
    for (const auto &entry : entries)
    {
        const auto &p = entry.profile;
        if ((!actorClass.empty() && !contains(p.actorClasses, actorClass)) ||
            (!itemId.empty() && !contains(p.itemIds, itemId)))
            continue;
        if (found && (found->profile.initialSpeed != p.initialSpeed ||
                      found->profile.maximumTravel != p.maximumTravel || found->profile.velocityKeys != p.velocityKeys))
            return {ProfileMatchStatus::Ambiguous, nullptr};
        if (!found)
            found = &entry;
    }
    if (!found)
        return {};
    return {found->curve.valid() ? ProfileMatchStatus::Matched : ProfileMatchStatus::Unsupported, found};
}
} // namespace mousebox::control
