#include "modes.hpp"
#include <array>
#include <stdexcept>
namespace mousebox::games::cs2
{
namespace
{
using V = AlgorithmVersion;
using F = ComparisonFactor;
constexpr std::array Modes{
    ModeDefinition{"v1", "", {V::V1, F::None}},
    ModeDefinition{"v2", "", {V::V2, F::None}},
    ModeDefinition{"v3", "", {V::V3, F::None}},
    ModeDefinition{"v4", "", {V::V4, F::None}},
    ModeDefinition{"compare-global-direction", "direction-reference", {V::V1, F::GlobalDirection}},
    ModeDefinition{"compare-no-p", "predictable-recoil", {V::V1, F::PredictableOff}},
    ModeDefinition{"compare-no-p-angle", "predictable-base-angle", {V::V1, F::PredictableAngleOff}},
    ModeDefinition{"compare-no-p-velocity", "predictable-angular-velocity", {V::V1, F::PredictableVelocityOff}},
    ModeDefinition{"compare-no-u", "unpredictable-recoil", {V::V1, F::UnpredictableOff}},
    ModeDefinition{"compare-no-target-velocity", "target-velocity", {V::V1, F::TargetVelocityOff}},
    ModeDefinition{"compare-no-self-velocity", "self-velocity", {V::V1, F::SelfVelocityOff}},
    ModeDefinition{"compare-no-sample-age", "sample-age-lead", {V::V1, F::SampleAgeOff}},
    ModeDefinition{"compare-no-response-lead", "response-delay-lead", {V::V1, F::ResponseLeadOff}},
    ModeDefinition{"compare-no-recoil-lead", "recoil-time-lead", {V::V1, F::RecoilLeadOff}},
    ModeDefinition{"compare-no-pending", "pending-motion-prediction", {V::V1, F::PendingPredictionOff}},
    ModeDefinition{"compare-no-gain", "tracking-gain-limit", {V::V1, F::GainLimitOff}},
    ModeDefinition{"compare-no-acceleration", "acceleration-limit", {V::V1, F::AccelerationOff}},
    ModeDefinition{"compare-no-braking", "braking-distance", {V::V1, F::BrakingOff}},
    ModeDefinition{"compare-no-variation", "learned-variation", {V::V1, F::VariationOff}},
    ModeDefinition{"compare-no-learned-limits", "learned-motion-limits", {V::V1, F::LearnedLimitsOff}},
};
} // namespace
std::span<const ModeDefinition> algorithmModes()
{
    return Modes;
}
const ModeDefinition &algorithmMode(ModeSelection selection)
{
    for (const auto &mode : Modes)
        if (mode.selection == selection)
            return mode;
    throw std::invalid_argument("未知CS2瞄准算法模式");
}
ModeSelection parseAlgorithmMode(std::string_view id)
{
    for (const auto &mode : Modes)
        if (mode.id == id)
            return mode.selection;
    throw std::invalid_argument("未知CS2瞄准算法模式");
}
PipelinePolicy pipelinePolicy(ModeSelection selection)
{
    algorithmMode(selection);
    PipelinePolicy p;
    if (selection.version == V::V4)
    {
        p.direction = DirectionReference::GlobalInput;
        p.nativeQualification = false;
        p.globalResponse = true;
    }
    switch (selection.comparison)
    {
    case F::None:
        break;
    case F::GlobalDirection:
        p.direction = DirectionReference::GlobalInput;
        break;
    case F::PredictableOff:
        p.recoil.predictable = false;
        break;
    case F::PredictableAngleOff:
        p.recoil.baseAngle = false;
        break;
    case F::PredictableVelocityOff:
        p.recoil.baseVelocity = false;
        break;
    case F::UnpredictableOff:
        p.recoil.unpredictable = false;
        break;
    case F::TargetVelocityOff:
        p.geometry.targetVelocity = false;
        break;
    case F::SelfVelocityOff:
        p.geometry.selfVelocity = false;
        break;
    case F::SampleAgeOff:
        p.time.sampleAge = false;
        break;
    case F::ResponseLeadOff:
        p.time.responseLead = false;
        break;
    case F::RecoilLeadOff:
        p.time.recoilLead = false;
        break;
    case F::PendingPredictionOff:
        p.subtractPending = false;
        break;
    case F::GainLimitOff:
        p.motion.gainLimit = false;
        break;
    case F::AccelerationOff:
        p.motion.acceleration = false;
        break;
    case F::BrakingOff:
        p.motion.braking = false;
        break;
    case F::VariationOff:
        p.motion.variation = false;
        break;
    case F::LearnedLimitsOff:
        p.motion.learnedLimits = false;
        break;
    }
    return p;
}
} // namespace mousebox::games::cs2
