#include "model.hpp"
#include "control/sample_time.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace mousebox::games::valorant::aim
{
namespace
{
bool validMapping(const control::movement::Vector &value) noexcept
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::abs(value.x) >= 1e-7 && std::abs(value.y) >= 1e-7 &&
           std::abs(value.x) <= 10 && std::abs(value.y) <= 10;
}
} // namespace
namespace
{
Solution sourceStatus(const Input &in, const Settings &settings, std::uint64_t now, bool requireForeground = true)
{
    Solution result;
    const auto blocked = [&](const char *reason) {
        result.block = reason;
        return result;
    };
    if (!std::isfinite(settings.coneDegrees) || settings.coneDegrees < .1 || settings.coneDegrees > 89 ||
        !std::isfinite(settings.maximumDistance) || settings.maximumDistance < 0 || settings.maximumAgeMs < 10 ||
        settings.maximumAgeMs > 1000 || settings.part >= 4 || settings.priority > 1 || settings.responseMs > 250)
        return blocked("invalid-settings");
    if (!in.frame)
        return result;
    const auto &frame = *in.frame;
    if (requireForeground && settings.checkForeground && !in.foreground)
        return blocked("waiting-foreground");
    if (frame.identity.reader.empty() || frame.identity.clockDomain.empty() || !frame.identity.epoch ||
        !frame.identity.localEntity || !frame.localResolved)
        return blocked("local-unavailable");
    double oldest = std::numeric_limits<double>::infinity(), newest = -oldest;
    for (unsigned index = 0; index < frame.samples.size(); ++index)
    {
        const auto &sample = frame.samples[index];
        if (sample.channel != index + 1 || !sample.current || sample.reader != frame.identity.reader ||
            sample.clockDomain != frame.identity.clockDomain || sample.epoch != frame.identity.epoch || !sample.capture)
            return blocked("source-mismatch");
        control::PredictionPolicy policy;
        policy.maximumAgeMs = settings.maximumAgeMs;
        const auto timing =
            control::estimateSampleTime(sample.sourceStartNs, sample.sourceFinishNs, sample.sourceClock, now, policy);
        if (!timing.usable())
            return blocked("source-timing-unavailable");
        oldest = std::min(oldest, timing.localSampleMs - timing.uncertaintyMs);
        newest = std::max(newest, timing.localSampleMs + timing.uncertaintyMs);
        result.ageMs = std::max(result.ageMs, timing.ageMs);
        if (index == 2)
            result.sampleMs = timing.localSampleMs - timing.uncertaintyMs;
    }
    // The streams stay independent; a bounded observation skew is required only for control.
    if (newest - oldest > std::min(50.0, static_cast<double>(settings.maximumAgeMs)))
        return blocked("source-skew");
    if (!finite(frame.origin) || !std::isfinite(frame.viewDegrees.x) || !std::isfinite(frame.viewDegrees.y) ||
        std::abs(std::remainder(frame.viewDegrees.y, 360.0)) > 90)
        return blocked("camera-unavailable");
    result.block.clear();
    return result;
}
} // namespace
bool canConfirmView(const Input &in, const Settings &settings, std::uint64_t now)
{
    // Confirmation is a configuration action inside the client window. Foreground
    // eligibility belongs to movement generation and its final commit check.
    return sourceStatus(in, settings, now, false).ready();
}
Solution solve(const Input &in, const Settings &settings, std::uint64_t now, std::optional<std::uint64_t> held)
{
    auto result = sourceStatus(in, settings, now);
    if (!result.ready())
        return result;
    const auto blocked = [&](const char *reason) {
        result.block = reason;
        return result;
    };
    if (!in.manualViewConfirmed)
        return blocked("manual-view-unconfirmed");
    if (!validMapping({settings.yawPerCount, settings.pitchPerCount}))
        return blocked("mapping-unavailable");
    const auto &frame = *in.frame;
    double best = std::numeric_limits<double>::infinity();
    bool present = false, dead = false;
    constexpr double toDegrees = 180.0 / std::numbers::pi;
    for (const auto &target : frame.targets)
    {
        if (held && target.id != *held)
            continue;
        present = true;
        if (target.alive == false)
        {
            dead = true;
            continue;
        }
        if (!target.id || target.id == frame.identity.localEntity ||
            !(target.relationship == 3 && settings.enemies || target.relationship == 2 && settings.teammates) ||
            !target.points[settings.part])
            continue;
        const auto point = *target.points[settings.part];
        if (!finite(point))
            continue;
        const auto dx = point.x - frame.origin.x, dy = point.y - frame.origin.y, dz = point.z - frame.origin.z;
        const auto distance = std::hypot(std::hypot(dx, dy), dz);
        if (distance <= 1e-7 || !std::isfinite(distance) ||
            settings.maximumDistance > 0 && distance > settings.maximumDistance)
            continue;
        const control::movement::Vector error{
            std::remainder(std::atan2(dy, dx) * toDegrees - frame.viewDegrees.x, 360.0),
            std::remainder(std::atan2(dz, std::hypot(dx, dy)) * toDegrees - frame.viewDegrees.y, 360.0)};
        const auto angular = std::hypot(error.x, error.y);
        if (angular > settings.coneDegrees || std::abs(error.x) >= 90 || std::abs(error.y) >= 90)
            continue;
        const auto score = settings.priority == 0 ? distance : angular;
        if (score >= best)
            continue;
        best = score;
        result.target = target.id;
        result.point = point;
        result.errorDegrees = error;
        result.angularDistance = angular;
        result.distance = distance;
        result.lifeKnown = target.alive.has_value();
    }
    if (!result.target)
        return blocked(held ? dead      ? "held-target-dead"
                              : present ? "held-target-unavailable"
                                        : "held-target-missing"
                            : "no-target");
    result.block.clear();
    return result;
}
} // namespace mousebox::games::valorant::aim
