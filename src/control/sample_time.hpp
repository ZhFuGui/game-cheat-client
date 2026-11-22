#pragma once
#include "domain/source_clock.hpp"
#include <optional>
namespace mousebox::control
{
enum class PredictionReason
{
    Applied,
    TotalWorldVelocityMissing,
    GeometryMissing,
    InvalidVelocity,
    SourceTimingInvalid,
    VelocitySampleSkew,
    ClockUnavailable,
    ClockStale,
    ClockUncertain,
    AgeOutOfRange
};
struct PredictionPolicy
{
    double maximumAgeMs = 250, maximumUncertaintyMs = 25, maximumVelocitySkewMs = 100;
    std::uint64_t maximumProbeAgeMs = 3000;
};
struct SampleTimeEstimate
{
    PredictionReason reason = PredictionReason::ClockUnavailable;
    double localSampleMs = 0, ageMs = 0, uncertaintyMs = 0;
    bool usable() const noexcept
    {
        return reason == PredictionReason::Applied;
    }
};
SampleTimeEstimate estimateSampleTime(std::int64_t startedNs, std::int64_t finishedNs,
                                      const std::optional<SourceClockEstimate> &, std::uint64_t nowMs,
                                      PredictionPolicy = {}) noexcept;
} // namespace mousebox::control
