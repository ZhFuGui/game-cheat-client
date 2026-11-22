#pragma once
#include "games/pubg/model/control_frame.hpp"
#include "control/sample_time.hpp"
#include "games/pubg/model/timing_settings.hpp"
namespace mousebox::control
{
struct PositionPrediction
{
    Vec3 position;
    PredictionReason reason = PredictionReason::TotalWorldVelocityMissing;
    double ageMs = 0, uncertaintyMs = 0;
    bool applied() const noexcept
    {
        return reason == PredictionReason::Applied;
    }
};
// Predict translation only, using the selected bone's real sampling interval.
// T1 is that interval's midpoint; T2 is consumer-local control time. Failed
// estimates return the original point instead of inventing velocity or age.
PositionPrediction predictPosition(const Target &, std::uint64_t nowMs, PredictionPolicy = {}) noexcept;
// Compare independently sampled camera and target at the same decision time.
PositionPrediction predictCameraPosition(const Frame &living, const Frame &view, std::uint64_t nowMs,
                                         double maximumVelocitySkewMs = DefaultLivingLocalSkewMs) noexcept;
} // namespace mousebox::control
