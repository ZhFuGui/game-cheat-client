#include "synchronization/consumer/projection.hpp"
#include "model/entity_collection.hpp"
#include "model/entities/body_skeleton.hpp"
#include "model/entities/field_presence.hpp"
#include "diagnostics/work_progress.hpp"
#include <algorithm>
#include <bit>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
namespace monitor::synchronization::consumer {
namespace {
constexpr unsigned Element = 0x80000000u;
Vector V(const Vec3& v) {
    return {v.x, v.y, v.z};
}
Origin O(const WorldOrigin& v) {
    return {v.x, v.y, v.z};
}
Rotation R(const ViewRotation& v) {
    return {v.pitch, v.yaw, v.roll};
}
Transform T(const monitor::Transform& v) {
    return {{v.rotation.x, v.rotation.y, v.rotation.z, v.rotation.w}, V(v.translation), V(v.scale)};
}
bool Current(const SampleInterval& s, const Snapshot& f) {
    return s.started != Clock::time_point{} && s.started >= f.started && s.started <= s.finished &&
           s.finished <= f.finished;
}
void Require(bool yes, const char* message) {
    if (!yes)
        throw std::invalid_argument(message);
}
FactStatus Status(const OwnedObservation& o) {
    if (o.value_status)
        return *o.value_status == OwnedReadStatus::Available ? FactStatus::Available : FactStatus::Partial;
    if (o.latest_status == OwnedReadStatus::NotAttempted)
        return FactStatus::NotAttempted;
    return o.issue.code == Code::ReadFailed || o.issue.code == Code::BackendUnavailable ||
                   o.issue.code == Code::InternalError
               ? FactStatus::ReadFailed
               : FactStatus::DependencyBlocked;
}
TokenKey Key(u64 tag, std::initializer_list<InstanceIdentity> objects) {
    TokenKey key{tag};
    for (const auto& i : objects) {
        key.push_back(i.actor_address);
        key.push_back(static_cast<u32>(i.internal_index));
        key.push_back(i.serial_number);
    }
    return key;
}
void Append(TokenKey& key, const std::optional<float>& v) {
    key.push_back(v.has_value());
    if (v)
        key.push_back(std::bit_cast<u32>(*v));
}
void Append(TokenKey& key, const std::optional<std::string>& s) {
    key.push_back(s.has_value());
    if (!s)
        return;
    key.push_back(s->size());
    for (size_t at = 0; at < s->size(); at += 8) {
        u64 word = 0;
        for (size_t n = 0; n < 8 && at + n < s->size(); ++n)
            word |= u64(static_cast<unsigned char>((*s)[at + n])) << (8 * n);
        key.push_back(word);
    }
}
struct Typed {
    const model::Character* character = nullptr;
    const VehicleSample* vehicle = nullptr;
    const PickupSample* pickup = nullptr;
    const CreatureSample* creature = nullptr;
    const ContainerSample* container = nullptr;
};
class Builder {
    const Snapshot& f;
    InstanceRegistry& ids;
    const ProjectionOptions& options;
    std::map<TokenKey, u64> token_ids;
    std::map<ContextFactGroup, const ContextFact*> context_facts;

  public:
    ProjectionResult out;
    Builder(const Snapshot& frame, u64 epoch, u64 revision, InstanceRegistry& registry,
            const ProjectionOptions& settings)
        : f(frame), ids(registry), options(settings) {
        auto& s = out.state;
        s.epoch = epoch;
        s.revision = revision;
        s.capture_sequence = f.sequence;
        s.capture = {f.started, f.finished};
        s.read_finished = f.read_finished;
        s.category = f.category;
        s.features = f.subscription_features;
    }
    u64 Token(TokenKey key) {
        const auto found = token_ids.find(key);
        if (found != token_ids.end())
            return found->second;
        const u64 symbol = token_ids.size() + 1;
        token_ids.emplace(key, symbol);
        out.tokens.emplace(symbol, std::move(key));
        return symbol;
    }
    u64 Use(EntityKind kind, const InstanceIdentity& identity, const SampleInterval& proof, bool direct = false) {
        Require(identity.actor_address && identity.internal_index >= 0 && Current(proof, f),
                "Consumer reference lacks current lifecycle closure");
        const auto id = ids.Resolve(kind, identity);
        const InstanceKey key{kind, identity};
        auto [it, added] = out.identities.emplace(id, IdentityUse{key, proof, direct});
        Require(added || it->second.key == key, "Consumer identity symbol collision");
        if (!added) {
            if (direct && !it->second.direct)
                it->second = {key, proof, true};
            else if (direct == it->second.direct) {
                it->second.interval.started = std::max(it->second.interval.started, proof.started);
                it->second.interval.finished = std::max(it->second.interval.finished, proof.finished);
            }
        }
        return id;
    }
    Reference Ref(EntityKind kind, const InstanceIdentity& i, const SampleInterval& proof) {
        return {Use(kind, i, proof), kind};
    }
    void Observe(GroupSample& group, std::vector<unsigned> path, const OwnedObservation& o) {
        SourceTiming timing;
        timing.status = Status(o);
        timing.capture_sequence = o.capture_sequence;
        timing.source = {o.started, o.finished};
        timing.source_category = f.category;
        if (o.value_status)
            Require(o.capture_sequence == f.sequence && Current(timing.source, f),
                    "Consumer nested dynamic value is not sampled in this batch");
        Require(group.sources.emplace(std::move(path), timing).second, "Duplicate Consumer business source path");
    }
    void Source(GroupSample& group, std::vector<unsigned> path, const SampleInterval& sample) {
        Require(Current(sample, f), "Consumer field source outside batch");
        Require(
            group.sources
                .emplace(std::move(path), SourceTiming{FactStatus::Available, f.sequence, sample, false, f.category})
                .second,
            "Duplicate Consumer field source path");
    }
    GroupSample& Local(ContextGroup group, FactStatus status, const SampleInterval& sample,
                       const SampleInterval& verified) {
        if (HasValues(status) || verified.started != Clock::time_point{} || verified.finished != Clock::time_point{})
            Require(Current(verified, f), "Consumer Local group lacks current closure");
        auto& target = out.state.context.local_groups[group];
        target.status = status;
        if (HasValues(status)) {
            Require(Current(sample, f) && sample.finished <= verified.finished,
                    "Consumer Local value source outside closure");
            target.source_capture_sequence = f.sequence;
            target.source = sample;
        }
        return target;
    }
    GroupSample& Local(ContextGroup group, const OwnedObservation& o, const SampleInterval& verified) {
        if (o.value_status)
            Require(o.capture_sequence == f.sequence, "Consumer Local observation belongs to another batch");
        return Local(group, Status(o), {o.started, o.finished}, verified);
    }
    template <class S> Spatial Position(const S& sample) {
        Spatial value;
        if (sample.position)
            value.position = V(*sample.position);
        if (sample.origin)
            value.origin = O(*sample.origin);
        return value;
    }
    ItemDefinition Definition(const OwnedItemInfo& sample) {
        ItemDefinition v;
        v.item_id = sample.item_id;
        v.display_name = sample.display_name;
        if (sample.identifier && sample.identifier->verified)
            v.identifier = sample.identifier->full;
        return v;
    }
    Stack ItemStack(const OwnedItemStackSample& s) { return {s.quantity, s.durability, s.max_durability}; }
    void ContextValues() {
        for (const auto& fact : f.current_facts->context_groups)
            Require(context_facts.emplace(fact.group, &fact).second, "Duplicate Consumer native context fact");
        auto& context = out.state.context;
        context.coordinate_convention = options.coordinate_convention;
        context.verified_meters_per_unit = options.verified_meters_per_unit;
        context.native_world_velocity_supported = true;
        context.pacing = {f.pipeline->target_fps, f.pipeline->pacing_revision, f.plan_revision};
        const auto source = [&](ContextFactGroup kind, unsigned native_field, ContextGroup, unsigned field) {
            const auto it = context_facts.find(kind);
            Require(it != context_facts.end(), "Missing Consumer native context source");
            const auto& fact = *it->second;
            if (!HasValues(fact.status) || !(fact.present_fields & (u64(1) << (native_field - 1))))
                return false;
            Require(Current(fact.verified, f), "Consumer context has no current closure");
            const ContextFieldSource* found = nullptr;
            for (const auto& sample : fact.field_sources)
                if (sample.field == native_field) {
                    Require(!found, "Duplicate Consumer context field source");
                    found = &sample;
                }
            Require(found && found->capture_sequence && found->sample.started != Clock::time_point{} &&
                        found->sample.started <= found->sample.finished && found->sample.finished <= f.finished &&
                        (found->reused || found->capture_sequence == f.sequence && found->sample.started >= f.started),
                    "Invalid Consumer context original field sample");
            context.sources.emplace(std::vector<unsigned>{field},
                                    SourceTiming{fact.status, found->capture_sequence, found->sample, found->reused,
                                                 found->reused ? DataCategory::None : f.category});
            return true;
        };
        if (source(ContextFactGroup::World, 2, ContextGroup::World, 1)) {
            Require(f.world != 0, "Consumer World source has no value");
            context.world_token = out.state.epoch;
        }
        if (source(ContextFactGroup::World, 6, ContextGroup::World, 2)) {
            Require(f.level != 0, "Consumer Level source has no value");
            context.level_token = out.state.epoch;
        }
        if (source(ContextFactGroup::World, 4, ContextGroup::World, 8)) {
            Require(f.game_state != 0, "Consumer GameState source has no value");
            context.game_state_token = out.state.epoch;
        }
        if (source(ContextFactGroup::Origin, 1, ContextGroup::Coordinates, 6)) {
            Require(f.origin.has_value(), "Consumer origin source has no value");
            context.origin = O(*f.origin);
        }
        if (!context.coordinate_convention.empty())
            context.sources.emplace(
                std::vector<unsigned>{4},
                SourceTiming{FactStatus::Available, f.sequence, {f.started, f.started}, false, f.category});
        if (context.verified_meters_per_unit)
            context.sources.emplace(
                std::vector<unsigned>{5},
                SourceTiming{FactStatus::Available, f.sequence, {f.started, f.started}, false, f.category});
        for (const auto path : {std::vector<unsigned>{7, 1}, std::vector<unsigned>{7, 2}, std::vector<unsigned>{7, 3},
                                std::vector<unsigned>{9}})
            context.sources.emplace(
                path, SourceTiming{FactStatus::Available, f.sequence, {f.started, f.started}, false, f.category});
        if (f.world_name_status == NameCaptureStatus::Succeeded && f.world_name.verified && f.name_world == f.world) {
            const SampleInterval sample{f.world_name_observed, f.world_name_finished};
            const bool current = f.world_name_capture_sequence == f.sequence && Current(sample, f);
            const bool directory = f.directory_generation && f.world_name_capture_sequence == f.directory_generation &&
                                   sample.started != Clock::time_point{} && sample.started <= sample.finished &&
                                   sample.started >= f.directory_sample.started && sample.finished <= f.started;
            Require(current || directory, "Consumer map name lacks matching world sample");
            context.map_name = f.world_name.full;
            context.sources.emplace(std::vector<unsigned>{3},
                                    SourceTiming{FactStatus::Available, f.world_name_capture_sequence, sample, !current,
                                                 current ? f.category : DataCategory::None});
        }
    }
    void LocalValues() {
        const auto& local = *f.local;
        const auto& camera = local.camera;
        std::optional<InstanceIdentity> controller_identity;
        SampleInterval controller_sample;
        if (camera.source_verified && camera.controller_identity && Current(camera.controller_sample, f) &&
            Current(camera.controller_verified, f)) {
            controller_identity = camera.controller_identity;
            controller_sample = camera.controller_sample;
        }
        if (local.controller_rotation && local.controller_rotation->observation.value_status &&
            local.controller_rotation->controller && Current(local.controller_rotation->controller_sample, f) &&
            Current(local.controller_rotation->verified, f)) {
            Require(!controller_identity || controller_identity == local.controller_rotation->controller,
                    "Consumer independent controller sources disagree");
            controller_identity = local.controller_rotation->controller;
            controller_sample = local.controller_rotation->controller_sample;
        }
        const auto control = context_facts.find(ContextFactGroup::LocalControl);
        Require(control != context_facts.end(), "Missing Consumer local-control fact");
        const auto& fact = *control->second;
        LocalControl value;
        GroupSample& control_group = out.state.context.local_groups[ContextGroup::LocalControl];
        control_group.status = fact.status;
        if (HasValues(fact.status)) {
            Require(Current(fact.verified, f), "Consumer local control lacks current closure");
            SampleInterval samples{};
            for (const auto& s : fact.field_sources) {
                Require(s.field >= 1 && s.field <= 64, "Invalid Consumer source field index");
                if (fact.present_fields & (u64(1) << (s.field - 1))) {
                    if (s.field == 7 || s.field == 5 || s.field == 4) {
                        Require(s.capture_sequence == f.sequence && !s.reused && Current(s.sample, f),
                                "Consumer local owner has no current original field");
                        if (samples.started == Clock::time_point{} || s.sample.started < samples.started)
                            samples.started = s.sample.started;
                        samples.finished = std::max(samples.finished, s.sample.finished);
                    }
                }
            }
            if (controller_identity)
                value.controller_token = Token(Key(1, {*controller_identity}));
            if (f.local_character_identity && (fact.present_fields & fields::context_local::LocalCharacter)) {
                Require(f.local_character_identity->actor_address == f.local_character,
                        "Consumer body contradicts original control source");
                value.character = Ref(EntityKind::Character, *f.local_character_identity, fact.verified);
            }
            if (f.self_pawn_identity && (fact.present_fields & fields::context_local::ControlledPawn) &&
                (f.self_pawn_kind == EntityKind::Character || f.self_pawn_kind == EntityKind::Vehicle)) {
                Require(f.self_pawn_identity->actor_address == f.self_pawn,
                        "Consumer controlled object contradicts source");
                value.controlled = Ref(f.self_pawn_kind, *f.self_pawn_identity, fact.verified);
            }
            if (samples.started == Clock::time_point{})
                samples = {fact.verified.started, fact.verified.started};
            control_group.source = samples;
            control_group.source_capture_sequence = f.sequence;
            control_group.value = value;
            if (value.controller_token)
                Source(control_group, {1}, controller_sample);
            for (const auto& s : fact.field_sources) {
                if (s.field == 7 && value.character)
                    Source(control_group, {2}, s.sample);
                if (s.field == 5 && value.controlled)
                    Source(control_group, {3}, s.sample);
            }
        }
        auto& pose = Local(ContextGroup::ViewPose, camera.pose_status, camera.camera_sample, camera.verified);
        if (HasValues(camera.pose_status)) {
            Require(camera.source_verified && camera.controller_identity && camera.manager_identity &&
                        Current(camera.controller_verified, f) && Current(camera.manager_verified, f),
                    "Consumer camera has no verified controller and source ownership");
            Camera c;
            c.controller_token = Token(Key(1, {*camera.controller_identity}));
            c.source_token = Token(Key(2, {*camera.controller_identity, *camera.manager_identity}));
            if (camera.location)
                c.position = V(*camera.location);
            if (camera.rotation)
                c.rotation = R(*camera.rotation);
            c.fov_degrees = camera.fov_degrees;
            c.cached_fov_degrees = camera.cached_fov_degrees;
            c.locked_fov_degrees = camera.locked_fov_degrees;
            c.cache_time_seconds = camera.camera_cache_time_seconds;
            if (camera.origin)
                c.origin = O(*camera.origin);
            pose.value = std::move(c);
            if (camera.location)
                Source(pose, {1}, camera.camera_sample);
            if (camera.rotation)
                Source(pose, {2}, camera.camera_sample);
            if (camera.fov_degrees)
                Source(pose, {3}, camera.camera_sample);
            if (camera.origin) {
                const auto origin = out.state.context.sources.find({6});
                Require(origin != out.state.context.sources.end(), "Consumer camera origin lacks its original sample");
                pose.sources.emplace(std::vector<unsigned>{4}, origin->second);
            }
            if (camera.cached_fov_degrees)
                Source(pose, {5}, camera.camera_sample);
            if (camera.locked_fov_degrees)
                Source(pose, {6}, camera.camera_sample);
            if (camera.camera_cache_time_seconds)
                Source(pose, {7}, camera.camera_sample);
            Source(pose, {8}, camera.controller_sample);
            Source(pose, {9}, camera.manager_sample);
        }
        auto& target =
            Local(ContextGroup::ViewTarget, camera.target_status, camera.target_sample, camera.target_verified);
        if (HasValues(camera.target_status)) {
            CameraTarget t;
            if (camera.target_address)
                t.has_target = *camera.target_address != 0;
            if (camera.pending_target_address)
                t.pending_active = *camera.pending_target_address != 0;
            if (camera.target_identity && camera.target_kind != EntityKind::Unknown) {
                Require(camera.target_address && *camera.target_address == camera.target_identity->actor_address,
                        "Consumer camera target identity contradicts association");
                t.target = Ref(camera.target_kind, *camera.target_identity, camera.target_verified);
            }
            if (t.has_target)
                Source(target, {1}, camera.target_sample);
            if (t.pending_active)
                Source(target, {2}, camera.target_sample);
            if (t.target)
                Source(target, {3}, camera.target_sample);
            target.value = std::move(t);
        }
        if (local.current_weapon) {
            const auto& current = *local.current_weapon;
            auto& weapon_group = Local(ContextGroup::LocalWeapon, current.observation, current.verified);
            if (current.observation.value_status) {
                Weapon w;
                w.has_current_item = current.has_current_item;
                w.active_slot = current.active_slot;
                if (current.character)
                    w.owner = Ref(EntityKind::Character, *current.character, current.verified);
                if (current.weapon) {
                    const auto& source = *current.weapon;
                    Observe(weapon_group, {2}, source.observation);
                    if (source.observation.value_status) {
                        Require(source.instance && source.object_address &&
                                    *source.object_address == source.instance->actor_address,
                                "Consumer weapon has no matching current instance");
                        w.weapon_token = Token(Key(3, {*source.instance}));
                        if (source.class_name && source.class_name->verified)
                            w.name = source.class_name->full;
                        if (source.muzzle) {
                            Observe(weapon_group, {4}, source.muzzle->observation);
                            if (source.muzzle->observation.value_status) {
                                const auto& m = *source.muzzle;
                                Muzzle v;
                                if (m.source)
                                    v.origin = O(m.source->origin);
                                if (m.attach_name && m.attach_name->verified)
                                    v.attach_name = m.attach_name->full;
                                if (m.bone_name && m.bone_name->verified)
                                    v.bone_name = m.bone_name->full;
                                if (m.binding)
                                    v.binding = u32(*m.binding);
                                if (m.world_transform)
                                    v.world_transform = T(*m.world_transform);
                                for (unsigned field : {1u, 2u, 3u, 6u, 10u}) {
                                    const bool present = field == 1   ? v.origin.has_value()
                                                         : field == 2 ? v.attach_name.has_value()
                                                         : field == 3 ? v.bone_name.has_value()
                                                         : field == 6 ? v.binding.has_value()
                                                                      : v.world_transform.has_value();
                                    if (present)
                                        Observe(weapon_group, {4, field}, m.observation);
                                }
                                w.muzzle = std::move(v);
                            }
                        }
                        if (source.recoil_targets) {
                            const auto& recoil = *source.recoil_targets;
                            Observe(weapon_group, {5}, recoil.observation);
                            if (recoil.observation.value_status)
                                w.recoil = Recoil{recoil.target_pitch, recoil.target_yaw, recoil.recovery_target};
                        }
                        if (source.trajectory_generation) {
                            const auto& generation = *source.trajectory_generation;
                            Observe(weapon_group, {6}, generation.observation);
                            if (generation.observation.value_status)
                                w.trajectory_generation = generation.counter;
                        }
                    }
                }
                if (w.weapon_token)
                    Observe(weapon_group, {1}, current.weapon->observation);
                if (w.active_slot)
                    Observe(weapon_group, {3}, current.observation);
                if (w.has_current_item)
                    Observe(weapon_group, {7}, current.observation);
                if (w.owner)
                    Observe(weapon_group, {8}, current.observation);
                weapon_group.value = std::move(w);
            }
        }
        const auto& audience = local.spectators;
        auto& spectators = Local(ContextGroup::LocalSpectators, audience.observation, audience.verified);
        if (audience.observation.value_status) {
            Spectators v;
            v.count = audience.count;
            if (audience.character)
                v.owner = Ref(EntityKind::Character, *audience.character, audience.verified);
            if (v.count)
                Observe(spectators, {1}, audience.observation);
            if (v.owner)
                Observe(spectators, {2}, audience.observation);
            spectators.value = std::move(v);
        }
        if (local.controller_rotation) {
            const auto& s = *local.controller_rotation;
            auto& g = Local(ContextGroup::LocalControllerRotation, s.observation, s.verified);
            if (s.observation.value_status) {
                Require(s.controller && (!camera.controller_identity || *s.controller == *camera.controller_identity),
                        "Consumer controller rotation contradicts the independently present camera controller");
                ControllerRotation v;
                v.controller_token = Token(Key(1, {*s.controller}));
                if (s.rotation)
                    v.rotation = R(*s.rotation);
                g.value = std::move(v);
                Source(g, {1}, s.controller_sample);
                Source(g, {2}, s.rotation_sample);
            }
        }
        if (local.native_holding) {
            const auto& s = *local.native_holding;
            auto& g = Local(ContextGroup::LocalNativeHolding, s.observation, s.verified);
            if (s.observation.value_status) {
                g.value = Holding{s.accepted_holding, s.holding_blend};
                if (s.accepted_holding)
                    Source(g, {3}, s.holding_sample);
                if (s.holding_blend)
                    Source(g, {4}, s.blend_sample);
            }
        }
        if (local.native_sway) {
            const auto& s = *local.native_sway;
            auto& g = Local(ContextGroup::LocalNativeSway, s.observation, s.verified);
            if (s.observation.value_status) {
                Sway v;
                if (s.rotation)
                    v.rotation = R(*s.rotation);
                if (v.rotation)
                    Observe(g, {3}, s.observation);
                g.value = std::move(v);
            }
        }
        if (local.aim_basis) {
            const auto& s = *local.aim_basis;
            auto& g = Local(ContextGroup::LocalAimBasis, s.observation, s.verified);
            if (s.observation.value_status) {
                if (!controller_identity) {
                    g = {};
                    g.status = FactStatus::DependencyBlocked;
                    return;
                }
                Require(s.character && s.weapon && s.gun_data &&
                            (s.branch != AimBasisBranch::Barrel || (s.mesh && s.trajectory_config)),
                        "Consumer AimBasis lacks original owner/resource/controller context");
                AimBasis v;
                v.owner = Ref(EntityKind::Character, *s.character, s.verified);
                v.weapon_token = Token(Key(3, {*s.weapon}));
                v.controller_token = Token(Key(1, {*controller_identity}));
                v.native_profile = s.native_profile;
                auto key = Key(4, {*s.character, *s.weapon, *controller_identity, *s.gun_data});
                for (const auto& resource : {s.mesh, s.trajectory_config}) {
                    key.push_back(resource.has_value());
                    if (resource) {
                        const auto part = Key(0, {*resource});
                        key.insert(key.end(), part.begin() + 1, part.end());
                    }
                }
                Append(key, s.native_profile);
                Append(key, s.zeroing_distance_m);
                key.push_back(s.branch ? u64(*s.branch) : 0);
                key.push_back(s.stage ? u64(*s.stage) : 0);
                key.push_back(s.native_zeroing_applied.has_value());
                if (s.native_zeroing_applied)
                    key.push_back(*s.native_zeroing_applied);
                v.native_context_token = Token(std::move(key));
                if (s.direction)
                    v.direction = V(*s.direction);
                v.zeroing_distance_m = s.zeroing_distance_m;
                v.zeroing_correction_degrees = s.zeroing_correction_degrees;
                v.integrated_steps = s.integrated_steps;
                v.zeroing_reached_distance = s.zeroing_reached_distance;
                v.integrated_distance_cm = s.integrated_distance_cm;
                if (s.branch)
                    v.branch = u32(*s.branch);
                if (s.stage)
                    v.stage = u32(*s.stage);
                v.native_zeroing_applied = s.native_zeroing_applied;
                // 这些业务值由整段已核定原生计算共同产生，保留真实依赖宽区间；不伪造字段级窄时刻。
                for (unsigned field : {1u, 2u, 3u, 12u, 13u, 14u, 15u})
                    Observe(g, {field}, s.observation);
                if (v.direction)
                    Observe(g, {6}, s.observation);
                if (v.zeroing_distance_m)
                    Observe(g, {7}, s.observation);
                if (v.zeroing_correction_degrees)
                    Observe(g, {8}, s.observation);
                if (v.integrated_steps)
                    Observe(g, {9}, s.observation);
                if (v.zeroing_reached_distance)
                    Observe(g, {10}, s.observation);
                if (v.integrated_distance_cm)
                    Observe(g, {11}, s.observation);
                Source(g, {16}, controller_sample);
                g.source.started = std::min(g.source.started, controller_sample.started);
                g.source.finished = std::max(g.source.finished, controller_sample.finished);
                g.value = std::move(v);
            }
        }
    }
    GroupValue Properties(const Typed& t, const ActorFact& fact, const GroupFact& attempt, GroupSample& g) {
        const auto mask = attempt.present_fields;
        const auto group = attempt.group;
        if (t.character) {
            const auto& c = *t.character;
            if (group == PropertyGroup::Spatial) {
                Spatial v;
                if (mask & fields::spatial::Position && c.position)
                    v.position = V(*c.position);
                if (mask & fields::spatial::Origin && c.origin)
                    v.origin = O(*c.origin);
                return v;
            }
            if (group == PropertyGroup::CharacterState) {
                LivingState v;
                if (mask & fields::character_state::Team)
                    v.team = c.team_id;
                if (mask & fields::character_state::Bot && c.bot_classification)
                    v.is_bot = c.bot_classification->is_bot;
                if (mask & fields::character_state::Type && c.character_type)
                    v.character_type = c.character_type->character_type;
                return v;
            }
            if (group == PropertyGroup::CharacterName)
                return Name{mask & fields::character_name::DisplayName ? c.display_name : std::nullopt};
            if (group == PropertyGroup::CharacterMovement) {
                Require(c.movement != nullptr, "Consumer movement has no sample");
                const auto& m = *c.movement;
                Movement v;
                if (mask & fields::character_movement::Velocity && m.observation.value_status && m.velocity)
                    v.component_velocity = V(*m.velocity);
                Observe(g, {1}, m.observation);
                if (mask & fields::character_movement::World && m.world) {
                    const auto& w = *m.world;
                    Observe(g, {4}, w.observation);
                    if (w.observation.value_status) {
                        WorldVelocity value;
                        if (w.actor) {
                            Require(*w.actor == *fact.identity,
                                    "Consumer total world velocity belongs to another character");
                            value.owner = Ref(EntityKind::Character, *w.actor, fact.lifecycle_verified);
                        }
                        if (w.velocity)
                            value.velocity = V(*w.velocity);
                        if (w.source)
                            value.source = u32(*w.source);
                        v.world = std::move(value);
                    }
                }
                return v;
            }
            if (group == PropertyGroup::CharacterSkeleton) {
                Require(c.skeleton && c.skeleton->source, "Consumer skeleton has no closed source");
                const auto& s = *c.skeleton;
                Skeleton v;
                v.owner = Ref(EntityKind::Character, *fact.identity, fact.lifecycle_verified);
                v.origin = O(s.source->origin);
                auto layout = Key(5, {s.source->skeletal_mesh});
                auto pose = Key(6, {s.source->mesh, s.source->skeletal_mesh, s.source->pose_component});
                std::set<u32> semantic_roles;
                for (const auto& bone : s.bones) {
                    if (!bone.semantic_role || *bone.semantic_role >= BodyBoneNames.size()) {
                        g.status = FactStatus::Partial;
                        continue;
                    }
                    const auto role = *bone.semantic_role;
                    if (!semantic_roles.insert(role).second) {
                        g.status = FactStatus::Partial;
                        continue;
                    }
                    layout.insert(layout.end(), {role, bone.index, u64(bone.parent_index ? *bone.parent_index : -1)});
                    pose.insert(pose.end(), {role, bone.index, u64(bone.pose_index)});
                    Bone b;
                    b.role = SemanticBone(role + 1);
                    if (BodyBoneParents[role] >= 0)
                        b.parent_role = SemanticBone(BodyBoneParents[role] + 1);
                    if (bone.world_position)
                        b.world_position = V(*bone.world_position);
                    v.bones.push_back(std::move(b));
                }
                std::sort(v.bones.begin(), v.bones.end(), [](const auto& a, const auto& b) { return a.role < b.role; });
                v.layout_token = Token(std::move(layout));
                v.pose_token = Token(std::move(pose));
                const auto origin = out.state.context.sources.find({6});
                Require(origin != out.state.context.sources.end(), "Consumer skeleton origin lacks original sample");
                g.sources.emplace(std::vector<unsigned>{1}, origin->second);
                Source(g, {3}, fact.lifecycle_verified);
                Observe(g, {4}, s.observation);
                Observe(g, {5}, s.observation);
                for (unsigned i = 0; i < v.bones.size(); ++i) {
                    Observe(g, {2, Element | i, 1}, s.observation);
                    if (v.bones[i].parent_role)
                        Observe(g, {2, Element | i, 2}, s.observation);
                    if (v.bones[i].world_position)
                        Source(g, {2, Element | i, 5}, g.source);
                }
                return v;
            }
        }
        if (t.vehicle) {
            const auto& s = *t.vehicle;
            if (group == PropertyGroup::Spatial)
                return Position(s.spatial);
            if (group == PropertyGroup::VehicleState) {
                Require(s.state.has_value(), "Consumer vehicle state has no sample");
                const auto& state = *s.state;
                VehicleState v;
                if (mask & fields::vehicle_state::Health)
                    v.health = state.health;
                if (mask & fields::vehicle_state::MaxHealth)
                    v.max_health = state.max_health;
                if (mask & fields::vehicle_state::Fuel)
                    v.fuel = state.fuel;
                if (mask & fields::vehicle_state::MaxFuel)
                    v.max_fuel = state.max_fuel;
                if (mask & fields::vehicle_state::Dying)
                    v.is_dying = state.is_dying;
                if (mask & fields::vehicle_state::Broken)
                    v.is_broken = state.is_broken;
                return v;
            }
            if (group == PropertyGroup::VehicleName)
                return Name{mask & fields::name::Text && s.name.verified ? std::optional(s.name.full) : std::nullopt};
            if (group == PropertyGroup::VehicleModel) {
                Require(s.model.has_value(), "Consumer vehicle model has no sample");
                VehicleModel v;
                if (mask & fields::vehicle_model::ModelKey)
                    v.model_key = s.model->model_key;
                if (mask & fields::vehicle_model::ClassName && s.model->class_name.verified)
                    v.class_name = s.model->class_name.full;
                return v;
            }
            if (group == PropertyGroup::VehicleClassification)
                return VehicleType{u32(s.kind)};
            if (group == PropertyGroup::VehicleSeats) {
                Require(s.seats.has_value(), "Consumer seats have no sample");
                const auto& seats = *s.seats;
                Seats v;
                v.coverage = u32(seats.coverage);
                if (mask & fields::seats::DeclaredSlots)
                    v.declared_seats = seats.declared_slots;
                if (mask & fields::seats::OccupiedSlots)
                    v.occupied_seats = seats.occupied_slots;
                for (const auto& seat : seats.seats) {
                    const auto i = unsigned(v.seats.size());
                    Seat value;
                    value.index = seat.seat_index;
                    if (seat.observation.value_status)
                        value.occupancy = u32(seat.occupancy);
                    Observe(g, {4, Element | i}, seat.observation);
                    if (seat.riders) {
                        value.coverage = u32(seat.riders->coverage);
                        for (const auto& rider : seat.riders->riders) {
                            if (!rider.observation.value_status || !rider.instance_identity)
                                continue;
                            Require(rider.pawn_address && *rider.pawn_address == rider.instance_identity->actor_address,
                                    "Consumer seat rider association contradicts lifecycle");
                            Observe(g, {4, Element | i, 4, Element | unsigned(value.riders.size())}, rider.observation);
                            value.riders.push_back(Ref(EntityKind::Character, *rider.instance_identity,
                                                       {rider.observation.started, rider.observation.finished}));
                        }
                    }
                    v.seats.push_back(std::move(value));
                }
                return v;
            }
        }
        if (t.pickup) {
            const auto& s = *t.pickup;
            if (group == PropertyGroup::Spatial)
                return Position(s.spatial);
            Require(s.item.has_value(), "Consumer pickup has no current item source");
            const auto& item = *s.item;
            if (group == PropertyGroup::PickupStack) {
                Require(item.stack.has_value(), "Consumer stack has no sample");
                return ItemStack(*item.stack);
            }
            if (group == PropertyGroup::PickupDefinition) {
                Require(item.item_info.has_value(), "Consumer item definition has no sample");
                return Definition(*item.item_info);
            }
            if (group == PropertyGroup::PickupItemClassification) {
                Require(item.item_info && item.item_info->classification &&
                            item.item_info->classification->value_status,
                        "Consumer item classification is not implemented");
                return ItemClassification{item.item_info->category, item.item_info->classification_status};
            }
        }
        if (t.creature) {
            const auto& s = *t.creature;
            if (group == PropertyGroup::Spatial)
                return Position(s.spatial);
            if (group == PropertyGroup::CreatureClassification)
                return CreatureType{s.character_type_raw ? std::optional<u32>(*s.character_type_raw) : std::nullopt,
                                    u32(s.species)};
        }
        if (t.container) {
            const auto& s = *t.container;
            if (group == PropertyGroup::ContainerClassification)
                return ContainerType{u32(s.kind)};
            if (group == PropertyGroup::ContainerContents) {
                Require(s.contents && s.contents->observation.value_status,
                        "Consumer ground contents have no closed sample");
                GroundContents v;
                v.coverage = u32(s.contents->coverage);
                for (const auto& member : s.contents->members) {
                    Require(member.observation.value_status && member.owner_actor == s.actor &&
                                member.object_internal_index && member.object_serial_number,
                            "Consumer ground member lacks current parent/lifecycle closure");
                    const auto index = unsigned(v.members.size());
                    std::vector<unsigned> path{2, Element | index};
                    Observe(g, path, member.observation);
                    GroundMember value;
                    value.id =
                        Use(EntityKind::Pickup,
                            {member.component_address, *member.object_internal_index, *member.object_serial_number},
                            {member.observation.started, member.observation.finished});
                    auto sub = path;
                    sub.push_back(2);
                    Observe(g, sub, member.spatial.observation);
                    if (member.spatial.observation.value_status)
                        value.spatial = Position(member.spatial);
                    if (member.item) {
                        const auto& item = *member.item;
                        if (item.item_info) {
                            sub = path;
                            sub.push_back(3);
                            Observe(g, sub, item.item_info->observation);
                            if (item.item_info->observation.value_status)
                                value.definition = Definition(*item.item_info);
                            if (item.item_info->classification) {
                                sub = path;
                                sub.push_back(5);
                                Observe(g, sub, *item.item_info->classification);
                                if (item.item_info->classification->value_status)
                                    value.classification = ItemClassification{item.item_info->category,
                                                                              item.item_info->classification_status};
                            }
                        }
                        if (item.stack) {
                            sub = path;
                            sub.push_back(4);
                            Observe(g, sub, item.stack->observation);
                            if (item.stack->observation.value_status)
                                value.stack = ItemStack(*item.stack);
                        }
                    }
                    v.members.push_back(std::move(value));
                }
                return v;
            }
        }
        throw std::invalid_argument("Consumer business group has no matching typed model");
    }
    void Entities() {
        std::unordered_map<u64, Typed> typed;
        const auto add = [&](u64 address, Typed t) {
            Require(address && typed.emplace(address, t).second, "Duplicate Consumer typed object");
        };
        const auto& collection = *f.prepared_entities;
        for (const auto& c : collection.characters)
            add(c.actor_id, {&c});
        for (const auto& v : collection.vehicles) {
            Require(bool(v.sample), "Consumer vehicle sample missing");
            add(v.sample->actor, {nullptr, v.sample.get()});
        }
        for (const auto& p : collection.pickups) {
            Require(bool(p.sample), "Consumer pickup sample missing");
            add(p.sample->actor, {nullptr, nullptr, p.sample.get()});
        }
        for (const auto& c : collection.creatures) {
            Require(bool(c.sample), "Consumer creature sample missing");
            add(c.sample->actor, {nullptr, nullptr, nullptr, c.sample.get()});
        }
        for (const auto& c : collection.containers) {
            Require(bool(c.sample), "Consumer ground sample missing");
            add(c.sample->actor, {nullptr, nullptr, nullptr, nullptr, c.sample.get()});
        }
        std::unordered_map<u64, const ActorFact*> facts;
        for (const auto& fact : f.current_facts->actors) {
            Require(facts.emplace(fact.actor_address, &fact).second, "Duplicate Consumer Actor fact");
            if (fact.admission != AdmissionState::Admitted)
                continue;
            Require(fact.identity && fact.identity->actor_address == fact.actor_address &&
                        fact.lifecycle_source_verified && fact.source_present_at_close &&
                        Current(fact.lifecycle_verified, f) && fact.verified_finished >= f.started &&
                        fact.verified_finished <= f.finished,
                    "Consumer Actor lacks current identity/source closure");
            const auto found = typed.find(fact.actor_address);
            Require(found != typed.end(), "Consumer admitted Actor has no typed sample");
            const auto& t = found->second;
            Require((fact.kind == EntityKind::Character) == bool(t.character) &&
                        (fact.kind == EntityKind::Vehicle) == bool(t.vehicle) &&
                        (fact.kind == EntityKind::Pickup) == bool(t.pickup) &&
                        (fact.kind == EntityKind::Creature) == bool(t.creature) &&
                        (fact.kind == EntityKind::Container) == bool(t.container),
                    "Consumer typed kind contradicts admission");
            const auto id = Use(fact.kind, *fact.identity, fact.lifecycle_verified, true);
            Entity entity{id, fact.kind, {}};
            std::set<PropertyGroup> seen;
            for (const auto& attempt : fact.groups) {
                Require(seen.insert(attempt.group).second, "Duplicate Consumer property attempt");
                Require((attempt.present_fields & ~AllowedFields(attempt.group)) == 0,
                        "Unknown native Consumer property mask");
                if (!consumer::GroupAllowed(fact.kind, attempt.group))
                    continue;
                GroupSample g;
                g.status = attempt.status;
                if (HasValues(attempt.status)) {
                    Require(Current(attempt.sample, f), "Consumer dynamic property has no current sample");
                    g.source = attempt.sample;
                    if (attempt.group == PropertyGroup::CharacterSkeleton) {
                        if (!t.character || !t.character->skeleton) {
                            g.status = FactStatus::DependencyBlocked;
                            entity.groups.emplace(attempt.group, std::move(g));
                            continue;
                        }
                        const auto& skeleton = *t.character->skeleton;
                        if (!skeleton.source || !skeleton.pose_sample || !Current(*skeleton.pose_sample, f)) {
                            // 布局或Pose前置失败只隔离本组；不能撤销其余人物属性或借用宽链时间。
                            g.status = FactStatus::DependencyBlocked;
                            entity.groups.emplace(attempt.group, std::move(g));
                            continue;
                        }
                        g.source = *skeleton.pose_sample;
                    }
                    g.source_capture_sequence = f.sequence;
                    g.value = Properties(t, fact, attempt, g);
                } else
                    Require(!attempt.present_fields, "Unavailable Consumer group retains values");
                entity.groups.emplace(attempt.group, std::move(g));
            }
            Require(out.state.entities.emplace(id, std::move(entity)).second, "Duplicate Consumer entity ID");
            out.entity_keys.emplace(id, InstanceKey{fact.kind, *fact.identity});
            diagnostics::CompletedCpuUnit();
        }
        const std::unordered_set<u64> closing(f.current_facts->closing_source_slots.begin(),
                                              f.current_facts->closing_source_slots.end());
        const auto scoped = ids.Scope(f.category);
        std::vector<InstanceKey> retire;
        for (const auto& [key, id] : scoped.Entries()) {
            if (out.identities.contains(id))
                continue;
            auto reason = closing.contains(key.instance.actor_address) ? RemovalReason::Unknown
                                                                       : RemovalReason::WithdrawnFromScope;
            if (const auto entry = facts.find(key.instance.actor_address); entry != facts.end()) {
                const auto& fact = *entry->second;
                if (fact.lifecycle_source_verified && fact.identity &&
                    fact.identity->actor_address == key.instance.actor_address && *fact.identity != key.instance) {
                    reason = RemovalReason::InstanceReplaced;
                    retire.push_back(key);
                } else if (fact.lifecycle_source_verified && fact.identity && *fact.identity == key.instance) {
                    if (fact.admission == AdmissionState::Retired) {
                        reason = RemovalReason::InstanceRetired;
                        retire.push_back(key);
                    } else if (fact.admission == AdmissionState::IdentityChanged) {
                        reason = RemovalReason::InstanceReplaced;
                        retire.push_back(key);
                    } else if (fact.admission == AdmissionState::Unadmitted)
                        reason = RemovalReason::ReadFailure;
                } else if (fact.admission == AdmissionState::Unadmitted)
                    reason = RemovalReason::ReadFailure;
            }
            out.state.removals[id] = reason;
        }
        for (const auto& key : retire)
            ids.Retire(key.kind, key.instance);
    }
    ProjectionResult Run() {
        Require(f.current_facts && f.current_facts->capture_sequence == f.sequence,
                "Consumer Snapshot has no matching facts");
        Require(!f.pipeline || f.pipeline->epoch == out.state.epoch, "Consumer epoch contradicts sealed batch");
        Require(f.pipeline && f.pipeline->target_interval_ns, "Consumer batch lacks frozen pacing");
        out.state.accepted = f.complete && !f.fatal && f.current_facts->frame_accepted &&
                             (f.category == DataCategory::Local
                                  ? bool(f.local)
                                  : f.current_facts->source_open_complete && f.current_facts->source_close_complete);
        ids.Fence(out.state.epoch);
        if (out.state.accepted) {
            Require(f.pipeline->tasks.Complete(), "Accepted Consumer batch has unexecuted property tasks");
            ContextValues();
            if (f.category == DataCategory::Local) {
                LocalValues();
                for (const auto group :
                     {ContextGroup::LocalControl, ContextGroup::ViewPose, ContextGroup::ViewTarget,
                      ContextGroup::LocalWeapon, ContextGroup::LocalSpectators, ContextGroup::LocalControllerRotation,
                      ContextGroup::LocalNativeHolding, ContextGroup::LocalNativeSway, ContextGroup::LocalAimBasis})
                    if (!out.state.context.local_groups.contains(group)) {
                        GroupSample absent;
                        absent.status = FactStatus::DependencyBlocked;
                        out.state.context.local_groups.emplace(group, std::move(absent));
                    }
            } else {
                Require(f.prepared_entities && f.current_facts->source_level == f.level,
                        "Consumer batch has no matching closed entity source");
                Require(f.current_facts->opening_source.started != Clock::time_point{} &&
                            f.current_facts->opening_source.started <= f.current_facts->opening_source.finished &&
                            f.current_facts->opening_source.finished <= f.finished &&
                            f.current_facts->closing_source.started != Clock::time_point{} &&
                            f.current_facts->closing_source.started <= f.current_facts->closing_source.finished &&
                            f.current_facts->closing_source.finished <= f.finished,
                        "Consumer Actor directory boundaries are invalid");
                Entities();
            }
        } else {
            const auto previous = ids.Scope(f.category);
            for (const auto& [key, id] : previous.Entries())
                out.state.removals[id] = RemovalReason::ReadFailure;
        }
        std::string error;
        if (!consumer::ValidateState(out.state, error))
            throw std::invalid_argument(error);
        return std::move(out);
    }
};
} // namespace
ProjectionResult ProjectCurrent(const Snapshot& frame, u64 epoch, u64 revision, InstanceRegistry& registry,
                                const ProjectionOptions& options) {
    auto staged = registry;
    auto result = Builder(frame, epoch, revision, staged, options).Run();
    registry = std::move(staged);
    return result;
}
} // namespace monitor::synchronization::consumer
