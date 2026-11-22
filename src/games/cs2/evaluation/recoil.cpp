#include "recoil.hpp"
#include <cs2_reader/v2/recoil.hpp>
#include <cmath>
#include <limits>

namespace mousebox::games::cs2
{
namespace
{
namespace reference = ::cs2_reader::v2::recoil;
reference::Angle angle(Angles value)
{
    return {static_cast<float>(value.x), static_cast<float>(value.y), static_cast<float>(value.z)};
}
std::optional<Angles> converted(std::optional<reference::Angle> value)
{
    if (!value || !std::isfinite(value->pitch) || !std::isfinite(value->yaw) || !std::isfinite(value->roll))
        return {};
    return Angles{value->pitch, value->yaw, value->roll};
}
std::optional<reference::Time> time(const std::optional<GameTime> &value)
{
    if (!value || !value->tick || !value->fraction)
        return {};
    return reference::Time{*value->tick, static_cast<float>(*value->fraction)};
}
std::optional<Angles> sum(const std::optional<Angles> &a, const std::optional<Angles> &b, double scale = 1)
{
    if (!a || !b)
        return {};
    const Angles value{(a->x + b->x) * scale, (a->y + b->y) * scale, (a->z + b->z) * scale};
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) ? std::optional(value)
                                                                                      : std::nullopt;
}
bool knownTimes(const AimPunchState &state)
{
    return state.times && state.times->worldGroup && state.times->secondsPerTick == reference::seconds_per_tick;
}
std::optional<Angles> predictable(const AimPunchState &state, const std::optional<GameTime> &target)
{
    if (!knownTimes(state) || !state.predictable || !state.predictable->baseAngle ||
        !state.predictable->baseAngularVelocity)
        return {};
    const auto origin = time(state.predictable->baseTime), selected = time(target);
    return origin && selected ? converted(reference::evaluate_component(angle(*state.predictable->baseAngle),
                                                                        angle(*state.predictable->baseAngularVelocity),
                                                                        *selected, *origin))
                              : std::nullopt;
}
std::optional<Angles> unpredictable(const AimPunchState &state, const std::optional<GameTime> &target)
{
    if (!knownTimes(state) || !state.unpredictable || !state.unpredictable->baseAngle || !state.unpredictable->baseTick)
        return {};
    const auto selected = time(target);
    return selected ? converted(reference::evaluate_component(angle(*state.unpredictable->baseAngle), {}, *selected,
                                                              {*state.unpredictable->baseTick, 0}))
                    : std::nullopt;
}
const Body *body(const Snapshot &frame, std::optional<std::uint32_t> handle)
{
    if (handle)
        for (const auto &player : frame.players)
            if (player.body && player.body->entity.id == *handle)
                return &*player.body;
    return nullptr;
}
bool directLocalPrediction(const Snapshot &frame, const Body &b)
{
    // The Reader contract marks this non-delegated, unparented input branch as
    // PredictionInput. Unlike native history lanes, its value does not
    // depend on the visual query selector recorded as native_context.
    if (!frame.local.controller || frame.local.pawn != b.entity.id ||
        frame.local.cameraRecoilPath != CameraRecoilPath::Player || frame.local.cameraRecoilPawn != frame.local.pawn ||
        b.entity.lifeState != 0 || b.entity.dormant != false || !b.eyeBasis || !validEyeBasis(*b.eyeBasis) ||
        b.eyeBasis->source != EyeAngleSource::PredictionInput || b.eyeBasis->interpolationParameter)
        return false;
    for (const auto &p : frame.players)
        if (p.controller == *frame.local.controller && p.local == true && p.body && p.body->entity.id == b.entity.id)
            return true;
    return false;
}
} // namespace
std::optional<Angles> evaluateComponent(Angles base, Angles velocity, double elapsedSeconds, bool scaleTwice)
{
    if (!std::isfinite(elapsedSeconds) || std::abs(elapsedSeconds) > std::numeric_limits<float>::max())
        return {};
    return converted(
        reference::evaluate_component(angle(base), angle(velocity), static_cast<float>(elapsedSeconds), scaleTwice));
}
RecoilEvaluation evaluateRecoil(const Snapshot &frame)
{
    RecoilEvaluation result;
    // Legacy diagnostic name: compatibility is established by the frozen
    // Reader contract and verified payload, never by a game build whitelist.
    result.supportedBuild = frame.directoryVerified;
    if (!result.supportedBuild)
        return result;
    const auto localBody = body(frame, frame.local.pawn);
    if (localBody && localBody->entity.health && *localBody->entity.health > 0 && localBody->aimPunch &&
        localBody->aimPunch->times)
    {
        const auto &punch = *localBody->aimPunch;
        result.shotPredictable = predictable(punch, punch.times->simulation);
        result.shotUnpredictable = unpredictable(punch, punch.times->simulation);
        result.shotOffset = sum(result.shotPredictable, result.shotUnpredictable);
        result.rawEyePlusRecoil = sum(localBody->networkEyeAngles, result.shotOffset);
        if (localBody->eyeBasis && validEyeBasis(*localBody->eyeBasis))
        {
            result.directionContextIndependent = directLocalPrediction(frame, *localBody);
            const bool matching = localBody->eyeBasis->nativeContext && punch.times->nativeContext &&
                                  localBody->eyeBasis->nativeContext == punch.times->nativeContext;
            if (matching || result.directionContextIndependent)
                result.contextDirection = sum(localBody->eyeBasis->value, result.shotOffset);
        }
        // No source for an observer's HUD recoil is declared by this contract.
        if (frame.local.cameraRecoilPath == CameraRecoilPath::Player &&
            frame.local.cameraRecoilPawn == frame.local.pawn && frame.local.crosshairRecoil.value_or(false))
            result.crosshairFollow = result.shotPredictable;
    }
    if (frame.local.cameraRecoilPath == CameraRecoilPath::Unspecified)
        return result;
    const auto cameraBody = body(frame, frame.local.cameraRecoilPawn);
    if (!cameraBody || !cameraBody->entity.health || *cameraBody->entity.health <= 0)
        return result;
    if (cameraBody->aimPunch && cameraBody->aimPunch->times &&
        cameraBody->aimPunch->times->visualPolicy != VisualRecoilPolicy::Unspecified)
    {
        const auto &punch = *cameraBody->aimPunch;
        result.cameraPredictable = predictable(punch, punch.times->visualPredictable);
        result.cameraUnpredictable = unpredictable(punch, punch.times->visualUnpredictable);
        result.cameraAim = sum(result.cameraPredictable, result.cameraUnpredictable, reference::camera_aim_fraction);
    }
    if (frame.local.viewPunch)
    {
        const auto &view = *frame.local.viewPunch;
        const auto origin = time(view.baseTime);
        if (view.baseAngle && origin && view.evaluationSeconds && view.decayExponent)
            result.cameraView = converted(reference::evaluate_view_punch(angle(*view.baseAngle), *origin,
                                                                         static_cast<float>(*view.evaluationSeconds),
                                                                         static_cast<float>(*view.decayExponent)));
    }
    result.cameraFeedback = sum(result.cameraAim, result.cameraView);
    return result;
}
std::optional<Angles> evaluateShotOffset(const AimPunchState &state, double ahead)
{
    if (!std::isfinite(ahead) || ahead < 0 || ahead > .75 || !state.times || !state.times->simulation)
        return {};
    auto selected = state.times->simulation;
    if (!selected->tick || !selected->fraction)
        return {};
    const double ticks = *selected->fraction + ahead / reference::seconds_per_tick;
    const double whole = std::floor(ticks);
    if (static_cast<double>(*selected->tick) + whole > std::numeric_limits<std::int32_t>::max())
        return {};
    selected->tick = static_cast<std::int32_t>(*selected->tick + whole);
    selected->fraction = ticks - whole;
    return sum(predictable(state, selected), unpredictable(state, selected));
}
} // namespace mousebox::games::cs2
