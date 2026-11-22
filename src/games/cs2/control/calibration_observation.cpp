#include "calibration_observation.hpp"
#include <algorithm>
#include <cmath>
namespace mousebox::games::cs2
{
CalibrationObservation observeCalibration(const ControlInput &in, const AimSettings &settings, std::uint64_t now)
{
    CalibrationObservation out;
    if (!in.current || !in.frame)
        return out;
    const auto &f = *in.frame;
    if (!f.directoryVerified)
    {
        out.block = "source-unverified";
        return out;
    }
    if (!localBody(f))
    {
        out.block = "local-not-playing";
        return out;
    }
    if (!f.local.angles || !std::isfinite(f.local.angles->x) || !std::isfinite(f.local.angles->y) ||
        !std::isfinite(f.local.angles->z))
    {
        out.block = "input-angles-missing";
        return out;
    }
    if (!f.local.mouseMapping || !nativeMouse(*f.local.mouseMapping))
    {
        out.block = "mouse-mapping-missing";
        return out;
    }
    if (!in.sample)
    {
        out.block = "clock-unavailable";
        return out;
    }
    if (!in.sample->current || in.sample->reader != f.instance || in.sample->clockDomain != f.clockDomain ||
        in.sample->epoch != f.epoch || in.sample->capture != f.sequence)
    {
        out.block = "sample-identity-mismatch";
        return out;
    }
    control::PredictionPolicy policy;
    policy.maximumAgeMs = settings.maximumAgeMs;
    out.timing = control::estimateSampleTime(in.sample->sourceStartNs, in.sample->sourceFinishNs,
                                             in.sample->sourceClock, std::max(now, in.observedAtMs), policy);
    if (!out.timing.usable())
    {
        switch (out.timing.reason)
        {
        case control::PredictionReason::ClockStale:
            out.block = "clock-stale";
            break;
        case control::PredictionReason::ClockUncertain:
            out.block = "clock-uncertain";
            break;
        case control::PredictionReason::AgeOutOfRange:
            out.block = "sample-age-exceeded";
            break;
        case control::PredictionReason::SourceTimingInvalid:
            out.block = "sample-timing-invalid";
            break;
        default:
            out.block = "clock-unavailable";
            break;
        }
        return out;
    }
    out.angles = f.local.angles;
    out.mapping = f.local.mouseMapping;
    out.block = "ready";
    return out;
}
} // namespace mousebox::games::cs2
