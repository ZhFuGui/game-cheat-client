#include "local_projection.hpp"
#include "games/pubg/model/native_aim_profiles.hpp"
#include <climits>
#include <cmath>
namespace mousebox::reader::detail
{
const s::Value *nativeField(const s::State &state, const s::GroupState &group, unsigned id, unsigned origin)
{
    const auto value = fresh(state, &group, id);
    const auto source = group.field_sources.find(id);
    if (!current(group, state.capture_sequence) || !value || source == group.field_sources.end() ||
        source->second.origin != origin || ns(source->second.sample.started) <= 0 ||
        source->second.sample.finished < source->second.sample.started ||
        source->second.sample.started < state.capture.started ||
        source->second.sample.finished > state.capture.finished || ns(group.verified_sample.started) <= 0 ||
        group.verified_sample.finished < group.verified_sample.started ||
        group.verified_sample.finished < source->second.sample.finished ||
        group.verified_sample.finished > state.capture.finished)
        return nullptr;
    return value;
}
control::NativeObservation nativeObservation(const s::State &state, const s::GroupState &group)
{
    control::NativeObservation result;
    result.status = static_cast<unsigned>(group.status);
    result.current = current(group, state.capture_sequence);
    result.capture = group.verified_capture_sequence;
    result.verifiedStartNs = ns(group.verified_sample.started);
    result.verifiedFinishNs = ns(group.verified_sample.finished);
    for (const auto &[id, source] : group.field_sources)
        if (nativeField(state, group, id, source.origin))
            result.fields.emplace(id, control::NativeFieldSource{ns(source.sample.started), ns(source.sample.finished),
                                                                 source.capture_sequence, source.origin});
    return result;
}
void projectNativeObservations(const s::State &state, control::Frame &frame)
{
    if (const auto found = state.context.find(s::ContextGroup::LocalWeapon); found != state.context.end())
    {
        const auto &group = found->second;
        control::CurrentItemSample sample;
        sample.observation = nativeObservation(state, group);
        sample.character = instance(fresh(state, &group, 1));
        if (frame.self && sample.character && sameInstance(*frame.self, *sample.character))
            sample.hasCurrentItem = boolean(nativeField(state, group, 7, 2));
        frame.currentItem = std::move(sample);
    }
    const auto ownerMatches = [&](const std::optional<control::Identity> &body,
                                  const std::optional<control::Identity> &weapon = {}) {
        return body && (!frame.self || sameInstance(*body, *frame.self)) &&
               (!weapon || !frame.activeWeapon || sameInstance(*weapon, frame.activeWeapon->instance));
    };
    if (const auto found = state.context.find(s::ContextGroup::LocalNativeHolding); found != state.context.end())
    {
        const auto &group = found->second;
        control::NativeHoldingSample sample;
        sample.observation = nativeObservation(state, group);
        sample.character = instance(nativeField(state, group, 1, 1));
        sample.component = instance(nativeField(state, group, 2, 1));
        if (ownerMatches(sample.character) && sample.component)
        {
            sample.accepted = boolean(nativeField(state, group, 3, 1));
            sample.blend = real(nativeField(state, group, 4, 1));
        }
        frame.nativeHolding = std::move(sample);
    }
    if (const auto found = state.context.find(s::ContextGroup::LocalNativeSway); found != state.context.end())
    {
        const auto &group = found->second;
        control::NativeSwaySample sample;
        sample.observation = nativeObservation(state, group);
        sample.character = instance(nativeField(state, group, 1, 1));
        sample.weapon = instance(nativeField(state, group, 2, 1));
        if (sample.weapon && ownerMatches(sample.character, sample.weapon))
            sample.rotation = vector(nativeField(state, group, 3, 2));
        frame.nativeSway = std::move(sample);
    }
    if (const auto found = state.context.find(s::ContextGroup::LocalAimBasis); found != state.context.end())
    {
        const auto &group = found->second;
        control::NativeAimBasisSample sample;
        sample.observation = nativeObservation(state, group);
        sample.character = instance(nativeField(state, group, 1, 1));
        sample.weapon = instance(nativeField(state, group, 2, 1));
        sample.mesh = instance(nativeField(state, group, 3, 1));
        sample.gunData = instance(nativeField(state, group, 4, 1));
        sample.trajectoryConfig = instance(nativeField(state, group, 5, 1));
        sample.zeroingDistanceMeters = real(nativeField(state, group, 7, 2));
        sample.zeroingCorrectionDegrees = real(nativeField(state, group, 8, 2));
        const auto branch = integer(nativeField(state, group, 12, 2)),
                   stage = integer(nativeField(state, group, 13, 2));
        sample.branch = branch && *branch <= UINT_MAX ? static_cast<unsigned>(*branch) : 0;
        sample.stage = stage && *stage <= UINT_MAX ? static_cast<unsigned>(*stage) : 0;
        const auto zeroingApplied = boolean(nativeField(state, group, 14, 2));
        sample.nativeZeroingApplied = zeroingApplied.value_or(false);
        sample.nativeProfile = text(nativeField(state, group, 15, 2));
        const auto steps = integer(nativeField(state, group, 9, 2));
        const auto reached = boolean(nativeField(state, group, 10, 2));
        const auto distance = real(nativeField(state, group, 11, 2));
        const bool integrationPresent = field(group.fields, 9) || field(group.fields, 10) || field(group.fields, 11);
        const bool integrationComplete = steps && *steps <= UINT32_MAX && reached && distance;
        if (integrationComplete)
        {
            sample.integratedSteps = static_cast<std::uint32_t>(*steps);
            sample.reachedDistance = reached;
            sample.integratedDistanceCm = distance;
        }
        const auto direction = vector(nativeField(state, group, 6, 2));
        const bool barrel = sample.mesh && sample.gunData && sample.trajectoryConfig && sample.zeroingDistanceMeters &&
                            sample.zeroingCorrectionDegrees && sample.branch == 1 && sample.stage == 1 &&
                            sample.nativeZeroingApplied && !sample.nativeProfile.empty() &&
                            (!integrationPresent || integrationComplete);
        // Candidate VIEW_NO_TRACE is an observation only. Explicit false is
        // mandatory; absent zeroing must not become a fabricated zero angle.
        const bool viewNoTrace = sample.branch == 2 && sample.stage == 1 && zeroingApplied && !*zeroingApplied &&
                                 control::knownViewNoTraceAimProfile(sample.nativeProfile) &&
                                 s::FieldMask(group.fields) == 0x782b && sample.gunData && !sample.mesh &&
                                 !sample.trajectoryConfig && !sample.zeroingDistanceMeters &&
                                 !sample.zeroingCorrectionDegrees && !integrationPresent;
        if (direction && sample.weapon && ownerMatches(sample.character, sample.weapon) && (barrel || viewNoTrace) &&
            std::abs(direction->x * direction->x + direction->y * direction->y + direction->z * direction->z - 1) <=
                1e-3)
            sample.direction = direction;
        frame.nativeAimBasis = std::move(sample);
    }
}
std::optional<control::ActiveWeapon> activeWeapon(const s::State &state, control::Identity owner)
{
    const auto currentWeapon = context(state, s::ContextGroup::LocalWeapon);
    if (!currentWeapon)
        return {};
    if (boolean(nativeField(state, *currentWeapon, 7, 2)) != std::optional<bool>{true})
        return {};
    const auto body = instance(fresh(state, currentWeapon, 1));
    const auto slot = integer(fresh(state, currentWeapon, 3)), slots = integer(fresh(state, currentWeapon, 6));
    const auto weapon = object(fresh(state, currentWeapon, 5));
    if (!body || !sameInstance(*body, owner) || !slot || *slot > INT32_MAX || !slots || *slot >= *slots || !weapon ||
        !nested(*currentWeapon, {5}, state.capture_sequence))
        return {};
    const auto key = instance(field(*weapon, 3));
    const auto address = integer(field(*weapon, 1));
    if (!key || !address || *address != key->actor)
        return {};
    control::ActiveWeapon next;
    next.owner = owner;
    next.instance = *key;
    next.slot = static_cast<int>(*slot);
    if (const auto name = object(field(*weapon, 2)))
        next.actorClass = text(field(*name, 5));
    const auto sample = nested(*currentWeapon, {5, 4}, state.capture_sequence);
    const auto muzzle = object(field(*weapon, 4));
    if (sample && muzzle)
    {
        const auto source = object(field(*muzzle, 1)), transform = object(field(*muzzle, 10));
        const auto origin = source ? vector(field(*source, 8)) : std::nullopt,
                   position = transform ? vector(field(*transform, 2)) : std::nullopt;
        if (origin && position)
            next.muzzle =
                control::MuzzleSample{*position, *origin, ns(sample->source.started), ns(sample->source.finished)};
    }
    if (const auto proof = nested(*currentWeapon, {5, 5}, state.capture_sequence))
        if (const auto value = object(field(*weapon, 5)))
        {
            const auto numeric = [&](unsigned id) {
                auto v = number(field(*value, id));
                return v && std::isfinite(*v) ? v : std::nullopt;
            };
            const auto pitch = numeric(1), yaw = numeric(2), recovery = numeric(3);
            if (pitch || yaw || recovery)
                next.recoilTargets = control::WeaponRecoilTargets{pitch, yaw, recovery, ns(proof->source.started),
                                                                  ns(proof->source.finished)};
        }
    if (const auto proof = nested(*currentWeapon, {5, 6}, state.capture_sequence))
        if (const auto value = object(field(*weapon, 6)))
        {
            const auto counter = integer(field(*value, 1));
            if (counter && *counter <= UINT32_MAX)
                next.trajectoryGeneration = control::WeaponTrajectoryGeneration{
                    static_cast<std::uint32_t>(*counter), ns(proof->source.started), ns(proof->source.finished)};
        }
    return next;
}
} // namespace mousebox::reader::detail
