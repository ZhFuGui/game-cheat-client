#pragma once
#include "geometry.hpp"
#include <cstdint>
#include <optional>

namespace mousebox::games::cs2
{
struct GameTime
{
    std::optional<std::int32_t> tick;
    std::optional<double> fraction;
};
enum class VisualRecoilPolicy : unsigned
{
    Unspecified,
    SeparateRender,
    SharedSimulation
};
struct RecoilTimes
{
    std::optional<std::uint32_t> worldGroup;
    VisualRecoilPolicy visualPolicy = VisualRecoilPolicy::Unspecified;
    std::optional<GameTime> simulation, visualPredictable, visualUnpredictable;
    std::optional<double> secondsPerTick;
    std::optional<std::int32_t> nativeContext;
};
struct PredictablePunch
{
    std::optional<GameTime> baseTime;
    std::optional<Angles> baseAngle, baseAngularVelocity;
};
struct UnpredictablePunch
{
    std::optional<std::int32_t> baseTick;
    std::optional<Angles> baseAngle;
};
struct AimPunchState
{
    std::optional<PredictablePunch> predictable;
    std::optional<UnpredictablePunch> unpredictable;
    std::optional<RecoilTimes> times;
};
struct CameraViewPunch
{
    std::optional<Angles> baseAngle;
    std::optional<GameTime> baseTime;
    std::optional<double> evaluationSeconds, decayExponent;
};
enum class NativeInputSource : unsigned
{
    Unspecified,
    Mouse,
    Analog
};
struct MouseMapping
{
    NativeInputSource source = NativeInputSource::Unspecified;
    std::optional<double> sensitivity, verticalScale, yaw, pitch, zoomRatio;
    std::optional<bool> invertedY;
    std::optional<double> effectivePitch, pawnFovAdjust, pawnMouseSensitivity, yawDegreesPerDelta, pitchDegreesPerDelta;
};
enum class CameraRecoilPath : unsigned
{
    Unspecified,
    Player,
    ObservedPlayer
};
enum class EyeAngleSource : unsigned
{
    Unspecified,
    Network,
    PredictionInput,
    NativeInterpolation,
    NativeHistory,
    ParentTransformedInput,
    Delegated,
    WorldRotation
};
struct EyeAngleBasis
{
    std::optional<Angles> value;
    EyeAngleSource source = EyeAngleSource::Unspecified;
    std::optional<std::int32_t> nativeContext;
    std::optional<double> interpolationParameter;
    std::optional<EyeAngleSource> resolvedSource;
    std::optional<std::uint32_t> sourceEntity;
    std::optional<unsigned> delegationDepth;
};
inline bool validEyeBasis(const EyeAngleBasis &basis)
{
    const auto source = static_cast<unsigned>(basis.source);
    if (!basis.value || source < 1 || source > 7)
        return false;
    if (basis.source != EyeAngleSource::Delegated)
        return !basis.resolvedSource && !basis.sourceEntity && !basis.delegationDepth;
    const auto resolved = static_cast<unsigned>(basis.resolvedSource.value_or(EyeAngleSource::Unspecified));
    return resolved >= 1 && resolved <= 7 && resolved != 6 && basis.sourceEntity && basis.delegationDepth &&
           *basis.delegationDepth > 0;
}
} // namespace mousebox::games::cs2
