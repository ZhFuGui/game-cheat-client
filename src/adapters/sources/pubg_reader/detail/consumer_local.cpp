#include "consumer_facts.hpp"
#include "games/pubg/model/native_aim_profiles.hpp"
#include <climits>
#include <cmath>

namespace mousebox::reader::consumer_detail
{
namespace
{
template <class T>
std::optional<T> take(const std::optional<T> &item, const c::GroupSample &group, unsigned id, const c::State &state)
{
    return item && field(group, id, state) ? item : std::nullopt;
}
void camera(const c::State &state, control::Frame &frame)
{
    const auto controlGroup = local(state, s::ContextGroup::LocalControl);
    std::optional<control::Identity> body;
    if (const auto control = value<c::LocalControl>(controlGroup, state))
    {
        frame.controller = take(control->controller_token, *controlGroup, 1, state);
        if (frame.controller && *frame.controller && field(*controlGroup, 2, state))
            body = character(control->character);
    }
    const auto poseGroup = local(state, s::ContextGroup::ViewPose);
    if (const auto pose = value<c::Camera>(poseGroup, state))
    {
        const auto controller = take(pose->controller_token, *poseGroup, 8, state);
        const auto sourceToken = take(pose->source_token, *poseGroup, 9, state);
        frame.cameraSourceAvailable = controller && *controller && sourceToken && *sourceToken;
        if (frame.controller != controller)
        {
            frame.controller.reset();
            body.reset();
        }
        if (frame.cameraSourceAvailable)
        {
            if (const auto position = take(pose->position, *poseGroup, 1, state))
                frame.camera = vector(*position);
            if (const auto rotation = take(pose->rotation, *poseGroup, 2, state))
                frame.rotation = vector(*rotation);
            frame.fov = take(pose->fov_degrees, *poseGroup, 3, state);
            if (const auto origin = take(pose->origin, *poseGroup, 4, state))
                frame.cameraOrigin = vector(*origin);
            else if (pose->origin && state.context.origin == pose->origin && contextField(state, 6))
            {
                const auto own = poseGroup->sources.find({4}), context = state.context.sources.find({6});
                if (own != poseGroup->sources.end() && context != state.context.sources.end() &&
                    own->second == context->second)
                    frame.cameraOrigin = vector(*pose->origin);
            }
            if (const auto timing = source(*poseGroup, {1}, state); timing && frame.camera)
            {
                frame.cameraSourceStartNs = ns(timing->source.started);
                frame.cameraSourceFinishNs = ns(timing->source.finished);
            }
            if (const auto timing = source(*poseGroup, {2}, state); timing && frame.rotation)
            {
                frame.rotationSourceStartNs = ns(timing->source.started);
                frame.rotationSourceFinishNs = ns(timing->source.finished);
            }
        }
    }
    frame.self = body;
}
void weapon(const c::State &state, control::Frame &frame)
{
    const auto group = local(state, s::ContextGroup::LocalWeapon);
    if (!group)
        return;
    control::CurrentItemSample currentItem;
    currentItem.observation = observation(*group, state);
    const auto item = value<c::Weapon>(group, state);
    if (item)
    {
        if (field(*group, 8, state))
            currentItem.character = character(item->owner);
        if (frame.self && currentItem.character == frame.self)
            currentItem.hasCurrentItem = take(item->has_current_item, *group, 7, state);
    }
    frame.currentItem = currentItem;
    if (!item || !frame.self || currentItem.character != frame.self ||
        currentItem.hasCurrentItem != std::optional<bool>(true))
        return;
    const auto token = take(item->weapon_token, *group, 1, state);
    const auto slot = take(item->active_slot, *group, 3, state);
    if (!token || !*token || !slot || *slot < 0)
        return;
    control::ActiveWeapon out;
    out.owner = *frame.self;
    out.instance = control::Identity::weapon(*token);
    out.slot = *slot;
    if (const auto name = take(item->name, *group, 2, state))
        out.actorClass = *name;
    if (const auto timing = source(*group, {4}, state);
        timing && item->muzzle && item->muzzle->origin && item->muzzle->world_transform)
        out.muzzle =
            control::MuzzleSample{vector(item->muzzle->world_transform->translation), vector(*item->muzzle->origin),
                                  ns(timing->source.started), ns(timing->source.finished)};
    if (const auto timing = source(*group, {5}, state); timing && item->recoil)
        out.recoilTargets = control::WeaponRecoilTargets{item->recoil->target_pitch, item->recoil->target_yaw,
                                                         item->recoil->recovery_target, ns(timing->source.started),
                                                         ns(timing->source.finished)};
    if (const auto timing = source(*group, {6}, state); timing && item->trajectory_generation)
        out.trajectoryGeneration = control::WeaponTrajectoryGeneration{
            *item->trajectory_generation, ns(timing->source.started), ns(timing->source.finished)};
    frame.activeWeapon = std::move(out);
}
void controller(const c::State &state, control::Frame &frame)
{
    const auto group = local(state, s::ContextGroup::LocalControllerRotation);
    if (const auto item = value<c::ControllerRotation>(group, state))
    {
        const auto token = take(item->controller_token, *group, 1, state);
        const auto timing = source(*group, {2}, state);
        if (token && *token && timing && item->rotation)
            frame.controllerRotation =
                control::ControllerRotationSample{control::Identity::controller(*token), vector(*item->rotation),
                                                  ns(timing->source.started), ns(timing->source.finished)};
    }
}
void observations(const c::State &state, control::Frame &frame)
{
    if (const auto group = local(state, s::ContextGroup::LocalNativeHolding))
    {
        control::NativeHoldingSample out;
        out.observation = observation(*group, state);
        if (const auto item = value<c::Holding>(group, state))
        {
            out.accepted = take(item->accepted_holding, *group, 3, state);
            out.blend = take(item->blend, *group, 4, state);
        }
        frame.nativeHolding = std::move(out);
    }
    if (const auto group = local(state, s::ContextGroup::LocalNativeSway))
    {
        control::NativeSwaySample out;
        out.observation = observation(*group, state);
        if (const auto item = value<c::Sway>(group, state))
            if (const auto rotation = take(item->rotation, *group, 3, state))
                out.rotation = vector(*rotation);
        frame.nativeSway = std::move(out);
    }
}
void aimBasis(const c::State &state, control::Frame &frame)
{
    const auto group = local(state, s::ContextGroup::LocalAimBasis);
    if (!group)
        return;
    control::NativeAimBasisSample out;
    out.observation = observation(*group, state);
    if (const auto item = value<c::AimBasis>(group, state))
    {
        if (field(*group, 1, state))
            out.character = character(item->owner);
        if (const auto token = take(item->weapon_token, *group, 2, state); token && *token)
            out.weapon = control::Identity::weapon(*token);
        out.nativeContextToken = take(item->native_context_token, *group, 3, state);
        out.controllerToken = take(item->controller_token, *group, 16, state);
        out.zeroingDistanceMeters = take(item->zeroing_distance_m, *group, 7, state);
        out.zeroingCorrectionDegrees = take(item->zeroing_correction_degrees, *group, 8, state);
        out.integratedSteps = take(item->integrated_steps, *group, 9, state);
        out.reachedDistance = take(item->zeroing_reached_distance, *group, 10, state);
        out.integratedDistanceCm = take(item->integrated_distance_cm, *group, 11, state);
        out.branch = take(item->branch, *group, 12, state).value_or(0);
        out.stage = take(item->stage, *group, 13, state).value_or(0);
        const auto applied = take(item->native_zeroing_applied, *group, 14, state);
        out.nativeZeroingApplied = applied.value_or(false);
        out.nativeProfile = take(item->native_profile, *group, 15, state).value_or("");
        const bool integrationPresent =
            item->integrated_steps || item->zeroing_reached_distance || item->integrated_distance_cm;
        const bool integrationComplete = out.integratedSteps && out.reachedDistance && out.integratedDistanceCm;
        const bool barrel = out.nativeContextToken && *out.nativeContextToken && out.controllerToken &&
                            *out.controllerToken && out.zeroingDistanceMeters && out.zeroingCorrectionDegrees &&
                            out.branch == 1 && out.stage == 1 && out.nativeZeroingApplied &&
                            !out.nativeProfile.empty() && (!integrationPresent || integrationComplete);
        const bool viewOnly = out.branch == 2 && out.stage == 1 && applied && !*applied &&
                              control::knownViewNoTraceAimProfile(out.nativeProfile) && !out.zeroingDistanceMeters &&
                              !out.zeroingCorrectionDegrees && !integrationPresent;
        const auto direction = take(item->direction, *group, 6, state);
        if (direction && out.character && frame.self && out.character == frame.self && out.weapon &&
            (!frame.activeWeapon || out.weapon == std::optional(frame.activeWeapon->instance)) && (barrel || viewOnly))
        {
            const auto v = vector(*direction);
            if (std::abs(v.x * v.x + v.y * v.y + v.z * v.z - 1) <= 1e-3)
                out.direction = v;
        }
    }
    frame.nativeAimBasis = std::move(out);
}
} // namespace
void projectLocal(const c::State &state, control::Frame &frame)
{
    camera(state, frame);
    weapon(state, frame);
    controller(state, frame);
    observations(state, frame);
    aimBasis(state, frame);
    const auto group = local(state, s::ContextGroup::LocalSpectators);
    if (const auto item = value<c::Spectators>(group, state))
    {
        const auto owner = character(item->owner);
        if (frame.self && owner == frame.self && field(*group, 2, state))
            if (const auto count = take(item->count, *group, 1, state); count && *count <= INT32_MAX)
                frame.spectatedCount = *count;
    }
}
} // namespace mousebox::reader::consumer_detail
