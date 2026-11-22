#include "games/pubg/pipeline/latency_prediction.hpp"
#include <algorithm>
#include <cmath>
namespace mousebox::control
{
PositionPrediction predictPosition(const Target &target, std::uint64_t now, PredictionPolicy policy) noexcept
{
    PositionPrediction result;
    if (!target.position || !finite(*target.position))
    {
        result.reason = PredictionReason::GeometryMissing;
        return result;
    }
    result.position = *target.position;
    if (!target.bodyWorldVelocity)
        return result;
    const auto &velocity = *target.bodyWorldVelocity;
    if (velocity.owner != target.identity || !finite(velocity.value) ||
        (velocity.source != BodyVelocitySource::ReaderTotalWorld &&
         velocity.source != BodyVelocitySource::NativeComponentAssumingZeroParent))
    {
        result.reason = PredictionReason::InvalidVelocity;
        return result;
    }
    if (target.sourceStartNs <= 0 || target.sourceFinishNs < target.sourceStartNs || velocity.sourceStartNs <= 0 ||
        velocity.sourceFinishNs < velocity.sourceStartNs)
    {
        result.reason = PredictionReason::SourceTimingInvalid;
        return result;
    }
    const double skew = static_cast<double>(std::max(
                            std::abs(static_cast<long double>(target.sourceStartNs - velocity.sourceStartNs)),
                            std::abs(static_cast<long double>(target.sourceFinishNs - velocity.sourceFinishNs)))) /
                        1e6;
    if (!std::isfinite(policy.maximumVelocitySkewMs) || policy.maximumVelocitySkewMs < 0 ||
        skew > policy.maximumVelocitySkewMs)
    {
        result.reason = PredictionReason::VelocitySampleSkew;
        return result;
    }
    const auto timing =
        estimateSampleTime(target.sourceStartNs, target.sourceFinishNs, target.sourceClock, now, policy);
    result.uncertaintyMs = timing.uncertaintyMs;
    result.ageMs = timing.ageMs;
    if (!timing.usable())
    {
        result.reason = timing.reason;
        return result;
    }
    const double seconds = result.ageMs / 1000;
    const Vec3 predicted{result.position.x + velocity.value.x * seconds, result.position.y + velocity.value.y * seconds,
                         result.position.z + velocity.value.z * seconds};
    if (!finite(predicted))
    {
        result.reason = PredictionReason::InvalidVelocity;
        return result;
    }
    result.position = predicted;
    result.reason = PredictionReason::Applied;
    return result;
}
PositionPrediction predictCameraPosition(const Frame &living, const Frame &view, std::uint64_t now,
                                         double maximumVelocitySkewMs) noexcept
{
    Target camera;
    camera.position = view.camera;
    camera.sourceStartNs = view.cameraSourceStartNs;
    camera.sourceFinishNs = view.cameraSourceFinishNs;
    camera.sourceClock = view.header.sourceClock;
    if (living.self)
    {
        camera.identity = *living.self;
        const auto own = std::find_if(living.characters.begin(), living.characters.end(),
                                      [&](const auto &c) { return c.identity == *living.self; });
        if (own != living.characters.end() && own->bodyWorldVelocity &&
            own->bodyWorldVelocity->source == BodyVelocitySource::ReaderTotalWorld)
            camera.bodyWorldVelocity = own->bodyWorldVelocity;
    }
    PredictionPolicy policy;
    policy.maximumVelocitySkewMs = maximumVelocitySkewMs;
    return predictPosition(camera, now, policy);
}
} // namespace mousebox::control
