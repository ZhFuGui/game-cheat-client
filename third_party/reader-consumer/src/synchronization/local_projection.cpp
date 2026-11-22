#include "synchronization/local_projection.hpp"
#include "synchronization/motion_weapon_projection.hpp"
#include <algorithm>
#include <stdexcept>

namespace monitor::synchronization {
namespace {
Value Instance(const InstanceIdentity& identity) {
    return Value(Value::Object{{1, Value(identity.actor_address)},
                               {2, Value(std::int64_t(identity.internal_index))},
                               {3, Value(u64(identity.serial_number))}});
}
Value Position(const Vec3& value) {
    return Value(Value::List{Value(double(value.x)), Value(double(value.y)), Value(double(value.z))});
}
Value Rotation(const ViewRotation& value) {
    return Value(Value::List{Value(double(value.pitch)), Value(double(value.yaw)), Value(double(value.roll))});
}
Value Origin(const WorldOrigin& value) {
    return Value(Value::List{Value(std::int64_t(value.x)), Value(std::int64_t(value.y)), Value(std::int64_t(value.z))});
}
bool Current(const SampleInterval& sample, const Snapshot& frame) {
    return sample.started != Clock::time_point{} && sample.started >= frame.started &&
           sample.started <= sample.finished && sample.finished <= frame.finished;
}
bool HasValues(FactStatus status) {
    return status == FactStatus::Available || status == FactStatus::Partial;
}
} // namespace
void ProjectCameraValues(const Snapshot& frame, State& out) {
    if (frame.category != DataCategory::Local || !frame.local || !frame.current_facts)
        throw std::invalid_argument("Missing current client view facts");
    const auto& view = frame.local->camera;
    const auto terminal = [&](ContextGroup kind, FactStatus status, const SampleInterval& verified) -> GroupState& {
        if (!Current(verified, frame))
            throw std::invalid_argument("View closing proof is outside this capture");
        auto& group = out.context[kind];
        group.status = status;
        group.verified_capture_sequence = frame.sequence;
        group.verified_sample = verified;
        return group;
    };
    const auto field = [&](GroupState& group, unsigned number, Value value, const SampleInterval& sample,
                           unsigned origin = 1) {
        if (!HasValues(group.status) || !Current(sample, frame) || sample.finished > group.verified_sample.finished)
            throw std::invalid_argument("View value has no current source and closing proof");
        group.fields.emplace(number, std::move(value));
        group.field_sources.emplace(
            number, GroupState::FieldSource{frame.sequence, sample, false, origin, DataCategory::Local});
    };
    if (!HasValues(view.source_status)) {
        terminal(ContextGroup::ViewSource, view.source_status, view.verified);
        terminal(ContextGroup::ViewPose, view.pose_status, view.verified);
        terminal(ContextGroup::ViewTarget, view.target_status, view.verified);
        return; // 相机失败组仍有终态；不撤销独立的控制与本地身体数据。
    }
    if (!view.source_verified || view.source_status != FactStatus::Available || !view.controller_identity ||
        !view.manager_identity || !Current(view.controller_verified, frame) || !Current(view.manager_verified, frame))
        throw std::invalid_argument("Client view has no verified controller and manager identity");
    const SampleInterval source_verified{std::min(view.controller_verified.started, view.manager_verified.started),
                                         std::max(view.controller_verified.finished, view.manager_verified.finished)};
    auto& source = terminal(ContextGroup::ViewSource, view.source_status, source_verified);
    field(source, 1, Value(view.controller), view.controller_sample);
    field(source, 2, Instance(*view.controller_identity), view.controller_sample);
    field(source, 3, Value(view.camera_manager), view.manager_sample);
    field(source, 4, Instance(*view.manager_identity), view.manager_sample);

    auto& pose = terminal(ContextGroup::ViewPose, view.pose_status, view.verified);
    if (HasValues(view.pose_status)) {
        if (view.location)
            field(pose, 1, Position(*view.location), view.camera_sample);
        if (view.rotation)
            field(pose, 2, Rotation(*view.rotation), view.camera_sample);
        if (view.fov_degrees)
            field(pose, 3, Value(double(*view.fov_degrees)), view.camera_sample, 2);
        if (view.camera_cache_time_seconds)
            field(pose, 5, Value(double(*view.camera_cache_time_seconds)), view.camera_sample);
        if (view.cached_fov_degrees)
            field(pose, 6, Value(double(*view.cached_fov_degrees)), view.camera_sample);
        if (view.locked_fov_degrees)
            field(pose, 7, Value(double(*view.locked_fov_degrees)), view.camera_sample);
        if (view.location && view.origin) {
            const ContextFact* origin = nullptr;
            for (const auto& fact : frame.current_facts->context_groups)
                if (fact.group == ContextFactGroup::Origin) {
                    if (origin)
                        throw std::invalid_argument("Duplicate view coordinate source fact");
                    origin = &fact;
                }
            if (!origin || origin->present_fields != 1 || origin->field_sources.size() != 1 ||
                origin->field_sources.front().field != 1 || origin->field_sources.front().reused ||
                origin->field_sources.front().capture_sequence != frame.sequence ||
                origin->field_sources.front().source != ContextSourceKind::Memory)
                throw std::invalid_argument("View origin has no own current memory sample");
            field(pose, 4, Origin(*view.origin), origin->field_sources.front().sample);
        }
    } else if (view.location || view.rotation || view.fov_degrees || view.camera_cache_time_seconds ||
               view.cached_fov_degrees || view.locked_fov_degrees)
        throw std::invalid_argument("Unavailable view pose retains old values");

    auto& target = terminal(ContextGroup::ViewTarget, view.target_status, view.target_verified);
    if (HasValues(view.target_status)) {
        if (view.target_address)
            field(target, 1, Value(*view.target_address), view.target_sample, 2);
        if (view.target_identity)
            field(target, 2, Instance(*view.target_identity), view.target_sample);
        if (view.current_target_address)
            field(target, 3, Value(*view.current_target_address), view.target_sample);
        if (view.pending_target_address)
            field(target, 4, Value(*view.pending_target_address), view.target_sample);
    } else if (view.target_address || view.target_identity || view.current_target_address ||
               view.pending_target_address)
        throw std::invalid_argument("Unavailable view target retains old values");
}
namespace {
FactStatus Status(const OwnedObservation& value) {
    if (value.value_status)
        return *value.value_status == OwnedReadStatus::Available ? FactStatus::Available : FactStatus::Partial;
    return value.issue.code == Code::ReadFailed || value.issue.code == Code::BackendUnavailable ||
                   value.issue.code == Code::InternalError
               ? FactStatus::ReadFailed
               : FactStatus::DependencyBlocked;
}
Value Name(const ObjectName& name) {
    Value::Object out;
    if (name.raw_index)
        out.emplace(1, Value(u64(*name.raw_index)));
    if (name.raw_number)
        out.emplace(2, Value(u64(*name.raw_number)));
    if (name.index)
        out.emplace(3, Value(u64(*name.index)));
    if (name.number)
        out.emplace(4, Value(u64(*name.number)));
    if (name.verified)
        out.emplace(5, Value(name.full));
    return Value(std::move(out));
}
void Observe(GroupState& group, std::vector<unsigned> path, const OwnedObservation& observation) {
    GroupState::NestedSample sample;
    sample.capture_sequence = observation.capture_sequence;
    sample.attempt = {observation.started, observation.finished};
    sample.latest_status = observation.latest_status;
    sample.value_status = observation.value_status;
    if (observation.value_status)
        sample.source = sample.attempt;
    group.nested_samples.emplace(std::move(path), sample);
}
} // namespace
void ProjectLocalValues(const Snapshot& frame, State& out) {
    ProjectCameraValues(frame, out);
    const auto& local = *frame.local;
    const auto group = [&](ContextGroup kind, const OwnedObservation& observation,
                           const SampleInterval& verified) -> GroupState& {
        if (!Current(verified, frame))
            throw std::invalid_argument("Local closing proof is outside capture");
        auto& value = out.context[kind];
        value.status = Status(observation);
        value.verified_capture_sequence = frame.sequence;
        value.verified_sample = verified;
        return value;
    };
    const auto field = [&](GroupState& group, unsigned key, Value value, const OwnedObservation& observation,
                           unsigned origin = 1) {
        const SampleInterval source{observation.started, observation.finished};
        if (!Current(source, frame) || source.finished > group.verified_sample.finished)
            throw std::invalid_argument("Local field is missing its current sampling interval");
        group.fields.emplace(key, std::move(value));
        group.field_sources.emplace(key,
                                    GroupState::FieldSource{frame.sequence, source, false, origin, DataCategory::Local});
    };
    if (!local.current_weapon)
        throw std::invalid_argument("Missing current local weapon terminal state");
    const auto& weapon = *local.current_weapon;
    auto& current = group(ContextGroup::LocalWeapon, weapon.observation, weapon.verified);
    if (weapon.observation.value_status) {
        if (weapon.character)
            field(current, 1, Instance(*weapon.character), weapon.observation);
        if (weapon.processor)
            field(current, 2, Instance(*weapon.processor), weapon.observation);
        if (weapon.active_slot)
            field(current, 3, Value(std::int64_t(*weapon.active_slot)), weapon.observation);
        if (weapon.has_current_item.has_value())
            field(current, 7, Value(*weapon.has_current_item), weapon.observation, 2);
        if (weapon.raw_weapon_value)
            field(current, 4, Value(*weapon.raw_weapon_value), weapon.observation);
        if (weapon.declared_slots)
            field(current, 6, Value(u64(*weapon.declared_slots)), weapon.observation);
        if (weapon.weapon) {
            const auto& source = *weapon.weapon;
            Value::Object value;
            if (source.observation.value_status) {
                if (source.object_address)
                    value.emplace(1, Value(*source.object_address));
                if (source.class_name)
                    value.emplace(2, Name(*source.class_name));
                if (source.instance)
                    value.emplace(3, Instance(*source.instance));
                if (source.muzzle) {
                    value.emplace(4, MuzzleValue(*source.muzzle));
                    Observe(current, {5, 4}, source.muzzle->observation);
                }
                if (source.recoil_targets) {
                    const auto& sample = *source.recoil_targets;
                    Value::Object targets;
                    if (sample.observation.value_status) {
                        if (sample.target_pitch)
                            targets.emplace(1, Value(double(*sample.target_pitch)));
                        if (sample.target_yaw)
                            targets.emplace(2, Value(double(*sample.target_yaw)));
                        if (sample.recovery_target)
                            targets.emplace(3, Value(double(*sample.recovery_target)));
                    }
                    value.emplace(5, Value(std::move(targets)));
                    Observe(current, {5, 5}, sample.observation);
                }
                if (source.trajectory_generation) {
                    const auto& sample = *source.trajectory_generation;
                    Value::Object generation;
                    if (sample.observation.value_status && sample.counter)
                        generation.emplace(1, Value(u64(*sample.counter)));
                    value.emplace(6, Value(std::move(generation)));
                    Observe(current, {5, 6}, sample.observation);
                }
            }
            field(current, 5, Value(std::move(value)), source.observation);
            Observe(current, {5}, source.observation);
        }
    }
    const auto& spectators = local.spectators;
    auto& audience = group(ContextGroup::LocalSpectators, spectators.observation, spectators.verified);
    if (spectators.observation.value_status) {
        if (spectators.character)
            field(audience, 1, Instance(*spectators.character), spectators.observation);
        if (spectators.count)
            field(audience, 2, Value(u64(*spectators.count)), spectators.observation);
    }
    if (local.controller_rotation) {
        const auto& sample = *local.controller_rotation;
        if (sample.observation.capture_sequence != frame.sequence)
            throw std::invalid_argument("Controller rotation belongs to another capture");
        if (!sample.observation.value_status && (sample.controller || sample.rotation))
            throw std::invalid_argument("Unavailable controller rotation retains old values");
        auto& rotation = group(ContextGroup::LocalControllerRotation, sample.observation, sample.verified);
        const auto source = [&](unsigned key, Value value, const SampleInterval& interval) {
            if (!Current(interval, frame) || interval.finished > sample.verified.finished)
                throw std::invalid_argument("Controller rotation has no current independent source interval");
            rotation.fields.emplace(key, std::move(value));
            rotation.field_sources.emplace(
                key, GroupState::FieldSource{frame.sequence, interval, false, 1, DataCategory::Local});
        };
        if (sample.observation.value_status) {
            if (sample.controller)
                source(1, Instance(*sample.controller), sample.controller_sample);
            if (sample.rotation)
                source(2, Rotation(*sample.rotation), sample.rotation_sample);
        }
    }
    const auto native_group = [&](ContextGroup kind, const OwnedObservation& observation,
                                  const SampleInterval& verified, bool retains_values) -> GroupState& {
        if (observation.capture_sequence != frame.sequence)
            throw std::invalid_argument("Native local observation belongs to another capture");
        if (!observation.value_status && retains_values)
            throw std::invalid_argument("Unavailable native local observation retains old values");
        if (observation.value_status && ((*observation.value_status != OwnedReadStatus::Available &&
                                          *observation.value_status != OwnedReadStatus::Partial) ||
                                         observation.latest_status != *observation.value_status))
            throw std::invalid_argument("Native local observation has an inconsistent value terminal");
        return group(kind, observation, verified);
    };
    const auto native_field = [&](GroupState& target, unsigned key, Value value, const SampleInterval& interval,
                                  const OwnedObservation& observation, unsigned origin = 1) {
        if (!Current(interval, frame) || interval.started < observation.started ||
            interval.finished > observation.finished || interval.finished > target.verified_sample.finished)
            throw std::invalid_argument("Native local field has no current independent source interval");
        target.fields.emplace(key, std::move(value));
        target.field_sources.emplace(
            key, GroupState::FieldSource{frame.sequence, interval, false, origin, DataCategory::Local});
    };
    if (local.native_holding) {
        const auto& sample = *local.native_holding;
        auto& holding = native_group(ContextGroup::LocalNativeHolding, sample.observation, sample.verified,
                                     sample.character || sample.component || sample.accepted_holding.has_value() ||
                                         sample.holding_blend.has_value());
        if (sample.observation.value_status) {
            const SampleInterval identities{sample.observation.started, sample.observation.finished};
            if (sample.character)
                native_field(holding, 1, Instance(*sample.character), identities, sample.observation);
            if (sample.component)
                native_field(holding, 2, Instance(*sample.component), identities, sample.observation);
            if (sample.accepted_holding.has_value())
                native_field(holding, 3, Value(*sample.accepted_holding), sample.holding_sample, sample.observation);
            if (sample.holding_blend.has_value())
                native_field(holding, 4, Value(double(*sample.holding_blend)), sample.blend_sample, sample.observation);
        }
    }
    if (local.native_sway) {
        const auto& sample = *local.native_sway;
        auto& sway = native_group(ContextGroup::LocalNativeSway, sample.observation, sample.verified,
                                  sample.character || sample.weapon || sample.rotation);
        if (sample.observation.value_status) {
            const SampleInterval source{sample.observation.started, sample.observation.finished};
            if (sample.character)
                native_field(sway, 1, Instance(*sample.character), source, sample.observation);
            if (sample.weapon)
                native_field(sway, 2, Instance(*sample.weapon), source, sample.observation);
            if (sample.rotation)
                native_field(sway, 3, Rotation(*sample.rotation), source, sample.observation, 2);
        }
    }
    if (local.aim_basis) {
        const auto& sample = *local.aim_basis;
        auto& basis = native_group(ContextGroup::LocalAimBasis, sample.observation, sample.verified,
                                   sample.character || sample.weapon || sample.mesh || sample.gun_data ||
                                       sample.trajectory_config || sample.direction || sample.zeroing_distance_m ||
                                       sample.zeroing_correction_degrees || sample.integrated_steps ||
                                       sample.zeroing_reached_distance.has_value() || sample.integrated_distance_cm ||
                                       sample.branch || sample.stage || sample.native_zeroing_applied.has_value() ||
                                       sample.native_profile);
        if (sample.observation.value_status) {
            const SampleInterval source{sample.observation.started, sample.observation.finished};
            for (const auto& [field, identity] :
                 {std::pair{1u, &sample.character}, std::pair{2u, &sample.weapon}, std::pair{3u, &sample.mesh},
                  std::pair{4u, &sample.gun_data}, std::pair{5u, &sample.trajectory_config}})
                if (*identity)
                    native_field(basis, field, Instance(**identity), source, sample.observation);
            if (sample.direction)
                native_field(basis, 6, Position(*sample.direction), source, sample.observation, 2);
            if (sample.zeroing_distance_m)
                native_field(basis, 7, Value(double(*sample.zeroing_distance_m)), source, sample.observation, 2);
            if (sample.zeroing_correction_degrees)
                native_field(basis, 8, Value(double(*sample.zeroing_correction_degrees)), source, sample.observation,
                             2);
            if (sample.integrated_steps)
                native_field(basis, 9, Value(u64(*sample.integrated_steps)), source, sample.observation, 2);
            if (sample.zeroing_reached_distance.has_value())
                native_field(basis, 10, Value(*sample.zeroing_reached_distance), source, sample.observation, 2);
            if (sample.integrated_distance_cm)
                native_field(basis, 11, Value(double(*sample.integrated_distance_cm)), source, sample.observation, 2);
            if (sample.branch)
                native_field(basis, 12, Value(u64(*sample.branch)), source, sample.observation, 2);
            if (sample.stage)
                native_field(basis, 13, Value(u64(*sample.stage)), source, sample.observation, 2);
            if (sample.native_zeroing_applied.has_value())
                native_field(basis, 14, Value(*sample.native_zeroing_applied), source, sample.observation, 2);
            if (sample.native_profile)
                native_field(basis, 15, Value(*sample.native_profile), source, sample.observation, 2);
        }
    }
}
} // namespace monitor::synchronization
