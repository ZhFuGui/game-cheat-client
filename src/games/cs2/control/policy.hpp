#pragma once
#include "control/movement/factors.hpp"
namespace mousebox::games::cs2
{
enum class AlgorithmVersion
{
    V1,
    V2,
    V3,
    V4
};
enum class ComparisonFactor
{
    None,
    GlobalDirection,
    PredictableOff,
    PredictableAngleOff,
    PredictableVelocityOff,
    UnpredictableOff,
    TargetVelocityOff,
    SelfVelocityOff,
    SampleAgeOff,
    ResponseLeadOff,
    RecoilLeadOff,
    PendingPredictionOff,
    GainLimitOff,
    AccelerationOff,
    BrakingOff,
    VariationOff,
    LearnedLimitsOff
};
struct ModeSelection
{
    AlgorithmVersion version = AlgorithmVersion::V1;
    ComparisonFactor comparison = ComparisonFactor::None;
    bool operator==(const ModeSelection &) const = default;
};
enum class DirectionReference
{
    NativeBasis,
    GlobalInput
};
struct TimeFactors
{
    bool sampleAge = true, responseLead = true, recoilLead = true;
};
struct RecoilFactors
{
    bool predictable = true, baseAngle = true, baseVelocity = true, unpredictable = true;
};
struct GeometryFactors
{
    bool targetVelocity = true, selfVelocity = true;
};
using MotionFactors = control::movement::Factors;
struct PipelinePolicy
{
    DirectionReference direction = DirectionReference::NativeBasis;
    bool nativeQualification = true, globalResponse = false, subtractPending = true;
    TimeFactors time;
    RecoilFactors recoil;
    GeometryFactors geometry;
    MotionFactors motion;
};
} // namespace mousebox::games::cs2
