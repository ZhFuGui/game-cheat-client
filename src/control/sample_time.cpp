#include "control/sample_time.hpp"
#include <algorithm>
#include <cmath>
namespace mousebox::control
{
SampleTimeEstimate estimateSampleTime(std::int64_t started, std::int64_t finished,
                                      const std::optional<SourceClockEstimate> &mapping, std::uint64_t now,
                                      PredictionPolicy policy) noexcept
{
    SampleTimeEstimate result;
    if (started <= 0 || finished < started)
    {
        result.reason = PredictionReason::SourceTimingInvalid;
        return result;
    }
    if (!mapping)
        return result;
    const auto &clock = *mapping;
    if (now < clock.calibratedAtMs || now - clock.calibratedAtMs > policy.maximumProbeAgeMs)
    {
        result.reason = PredictionReason::ClockStale;
        return result;
    }
    result.uncertaintyMs = clock.uncertaintyMs + static_cast<double>(finished - started) / 2e6;
    if (clock.sourceAnchorNs <= 0 || !std::isfinite(clock.localAnchorMs) || !std::isfinite(clock.uncertaintyMs) ||
        clock.uncertaintyMs < 0 || !std::isfinite(policy.maximumUncertaintyMs) || policy.maximumUncertaintyMs < 0 ||
        !std::isfinite(result.uncertaintyMs) || result.uncertaintyMs > policy.maximumUncertaintyMs)
    {
        result.reason = PredictionReason::ClockUncertain;
        return result;
    }
    const long double relative =
        static_cast<long double>(started - clock.sourceAnchorNs) + static_cast<long double>(finished - started) / 2;
    const long double sampled = clock.localAnchorMs + relative / 1e6L, age = static_cast<long double>(now) - sampled;
    if (!std::isfinite(policy.maximumAgeMs) || policy.maximumAgeMs <= 0 || !std::isfinite(age) ||
        age + result.uncertaintyMs < 0 || age + result.uncertaintyMs > policy.maximumAgeMs)
    {
        result.reason = PredictionReason::AgeOutOfRange;
        return result;
    }
    result.localSampleMs = static_cast<double>(sampled);
    result.ageMs = static_cast<double>(std::max(0.0L, age));
    result.reason = PredictionReason::Applied;
    return result;
}
} // namespace mousebox::control
