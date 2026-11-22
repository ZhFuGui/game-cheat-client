#include "synchronization/consumer/state.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <type_traits>
#include "model/entities/body_skeleton.hpp"
namespace monitor::synchronization::consumer {
namespace {
bool Interval(const SampleInterval& a, const SampleInterval& b) noexcept {
    return a.started == b.started && a.finished == b.finished;
}
bool ValidInterval(const SampleInterval& i, const SampleInterval& capture, bool reused = false) noexcept {
    return i.started != Clock::time_point{} && i.started <= i.finished && i.finished <= capture.finished &&
           (reused || i.started >= capture.started);
}
bool Valid(const Vector& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
bool Valid(const Rotation& v) {
    return std::isfinite(v.pitch) && std::isfinite(v.yaw) && std::isfinite(v.roll);
}
bool Valid(const Quaternion& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) && std::isfinite(v.w);
}
bool Valid(const Transform& v) {
    return Valid(v.rotation) && Valid(v.translation) && Valid(v.scale);
}
bool Valid(const Reference& r) {
    return r.id && r.kind >= EntityKind::Character && r.kind <= EntityKind::Container;
}
bool Valid(const Origin&) {
    return true;
}
bool Valid(const std::string& s) {
    for (size_t i = 0; i < s.size();) {
        const auto c = static_cast<unsigned char>(s[i++]);
        if (c < 0x80)
            continue;
        unsigned count = 0;
        u32 value = 0, min = 0;
        if (c >= 0xc2 && c <= 0xdf) {
            count = 1;
            value = c & 31;
            min = 0x80;
        } else if (c >= 0xe0 && c <= 0xef) {
            count = 2;
            value = c & 15;
            min = 0x800;
        } else if (c >= 0xf0 && c <= 0xf4) {
            count = 3;
            value = c & 7;
            min = 0x10000;
        } else
            return false;
        if (count > s.size() - i)
            return false;
        for (unsigned n = 0; n < count; ++n) {
            const auto t = static_cast<unsigned char>(s[i++]);
            if ((t & 0xc0) != 0x80)
                return false;
            value = (value << 6) | (t & 63);
        }
        if (value < min || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
            return false;
    }
    return true;
}
bool Valid(const Spatial&);
bool Valid(const LivingState&);
bool Valid(const Name&);
bool Valid(const WorldVelocity&);
bool Valid(const Movement&);
bool Valid(const Skeleton&);
bool Valid(const VehicleState&);
bool Valid(const Seats&);
bool Valid(const VehicleModel&);
bool Valid(const VehicleType&);
bool Valid(const CreatureType&);
bool Valid(const ContainerType&);
bool Valid(const ItemDefinition&);
bool Valid(const ItemClassification&);
bool Valid(const Stack&);
bool Valid(const GroundContents&);
bool Valid(const LocalControl&);
bool Valid(const Camera&);
bool Valid(const CameraTarget&);
bool Valid(const Muzzle&);
bool Valid(const Recoil&);
bool Valid(const Weapon&);
bool Valid(const Spectators&);
bool Valid(const ControllerRotation&);
bool Valid(const Holding&);
bool Valid(const Sway&);
bool Valid(const AimBasis&);
template <class T> bool Valid(const std::optional<T>& v) {
    if (!v)
        return true;
    if constexpr (std::is_floating_point_v<T>)
        return std::isfinite(*v);
    else if constexpr (std::is_arithmetic_v<T>)
        return true;
    else
        return Valid(*v);
}
bool Valid(const Spatial& v) {
    return Valid(v.position) && Valid(v.origin);
}
bool Valid(const LivingState& v) {
    return !v.character_type || (*v.character_type >= 1 && *v.character_type <= 9);
}
bool Valid(const Name& v) {
    return Valid(v.value);
}
bool Valid(const WorldVelocity& v) {
    return Valid(v.owner) && Valid(v.velocity) && (!v.source || (*v.source >= 1 && *v.source <= 5));
}
bool Valid(const Movement& v) {
    return Valid(v.component_velocity) && Valid(v.world);
}
bool Valid(const Skeleton& v) {
    if (!Valid(v.owner) || v.layout_token && !*v.layout_token || v.pose_token && !*v.pose_token || !Valid(v.origin) ||
        v.bones.size() > 20)
        return false;
    std::set<SemanticBone> seen;
    for (const auto& b : v.bones)
        if (unsigned(b.role) < 1 || unsigned(b.role) > 20 ||
            b.parent_role &&
                (unsigned(*b.parent_role) < 1 || unsigned(*b.parent_role) > 20 || *b.parent_role == b.role) ||
            !seen.insert(b.role).second || !Valid(b.world_position))
            return false;
    return true;
}
bool Valid(const VehicleState& v) {
    return Valid(v.health) && Valid(v.max_health) && Valid(v.fuel) && Valid(v.max_fuel);
}
bool Valid(const Seats& v) {
    if (v.coverage > 4)
        return false;
    std::set<i32> indices;
    for (const auto& s : v.seats) {
        if (s.occupancy > 2 || s.coverage > 4 || s.index && (!indices.insert(*s.index).second || *s.index < 0))
            return false;
        std::set<u64> riders;
        for (const auto& r : s.riders)
            if (!Valid(r) || r.kind != EntityKind::Character || !riders.insert(r.id).second)
                return false;
    }
    return true;
}
bool Valid(const VehicleModel& v) {
    return Valid(v.model_key) && Valid(v.class_name);
}
bool Valid(const VehicleType& v) {
    return v.kind <= 5;
}
bool Valid(const CreatureType& v) {
    return v.species <= 1 && (!v.character_type || (*v.character_type >= 1 && *v.character_type <= 9));
}
bool Valid(const ContainerType& v) {
    return v.kind <= 4;
}
bool Valid(const ItemDefinition& v) {
    return Valid(v.identifier) && Valid(v.display_name);
}
bool Valid(const ItemClassification&) {
    return true;
}
bool Valid(const Stack& v) {
    return Valid(v.durability) && Valid(v.max_durability);
}
bool Valid(const GroundContents& v) {
    if (v.coverage > 4)
        return false;
    std::set<u64> ids;
    for (const auto& m : v.members)
        if (!m.id || !ids.insert(m.id).second || !Valid(m.spatial) || !Valid(m.definition) || !Valid(m.stack) ||
            !Valid(m.classification))
            return false;
    return true;
}
bool Valid(const LocalControl& v) {
    return (!v.controller_token || *v.controller_token) && Valid(v.character) && Valid(v.controlled);
}
bool Valid(const Camera& v) {
    return (!v.controller_token || *v.controller_token) && (!v.source_token || *v.source_token) && Valid(v.position) &&
           Valid(v.rotation) && Valid(v.origin) && Valid(v.fov_degrees) && Valid(v.cached_fov_degrees) &&
           Valid(v.locked_fov_degrees) && Valid(v.cache_time_seconds) &&
           (!v.fov_degrees || *v.fov_degrees > 0 && *v.fov_degrees < 180);
}
bool Valid(const CameraTarget& v) {
    return Valid(v.target) && (!v.target || v.has_target && *v.has_target);
}
bool Valid(const Muzzle& v) {
    return Valid(v.origin) && Valid(v.attach_name) && Valid(v.bone_name) && Valid(v.world_transform) &&
           (!v.binding || *v.binding >= 1 && *v.binding <= 3);
}
bool Valid(const Recoil& v) {
    return Valid(v.target_pitch) && Valid(v.target_yaw) && Valid(v.recovery_target);
}
bool Valid(const Weapon& v) {
    return Valid(v.owner) && (!v.weapon_token || *v.weapon_token) && Valid(v.name) && Valid(v.muzzle) &&
           Valid(v.recoil) &&
           (!v.has_current_item || *v.has_current_item ||
            !v.weapon_token && !v.name && !v.muzzle && !v.recoil && !v.trajectory_generation);
}
bool Valid(const Spectators& v) {
    return Valid(v.owner);
}
bool Valid(const ControllerRotation& v) {
    return (!v.controller_token || *v.controller_token) && Valid(v.rotation);
}
bool Valid(const Holding& v) {
    return Valid(v.blend);
}
bool Valid(const Sway& v) {
    return Valid(v.rotation);
}
bool Valid(const AimBasis& v) {
    return (!v.native_context_token || *v.native_context_token) && (!v.controller_token || *v.controller_token) &&
           (!v.weapon_token || *v.weapon_token) && Valid(v.owner) && Valid(v.native_profile) && Valid(v.direction) &&
           Valid(v.zeroing_distance_m) && Valid(v.zeroing_correction_degrees) && Valid(v.integrated_distance_cm) &&
           (!v.branch || *v.branch >= 1 && *v.branch <= 2) && (!v.stage || *v.stage == 1);
}
bool Type(PropertyGroup g, const GroupValue& v) {
    switch (g) {
        case PropertyGroup::Spatial:
            return std::holds_alternative<Spatial>(v);
        case PropertyGroup::CharacterState:
            return std::holds_alternative<LivingState>(v);
        case PropertyGroup::CharacterName:
        case PropertyGroup::VehicleName:
            return std::holds_alternative<Name>(v);
        case PropertyGroup::CharacterMovement:
            return std::holds_alternative<Movement>(v);
        case PropertyGroup::CharacterSkeleton:
            return std::holds_alternative<Skeleton>(v);
        case PropertyGroup::VehicleState:
            return std::holds_alternative<VehicleState>(v);
        case PropertyGroup::VehicleSeats:
            return std::holds_alternative<Seats>(v);
        case PropertyGroup::VehicleModel:
            return std::holds_alternative<VehicleModel>(v);
        case PropertyGroup::VehicleClassification:
            return std::holds_alternative<VehicleType>(v);
        case PropertyGroup::CreatureClassification:
            return std::holds_alternative<CreatureType>(v);
        case PropertyGroup::PickupStack:
            return std::holds_alternative<Stack>(v);
        case PropertyGroup::PickupDefinition:
            return std::holds_alternative<ItemDefinition>(v);
        case PropertyGroup::PickupItemClassification:
            return std::holds_alternative<ItemClassification>(v);
        case PropertyGroup::ContainerClassification:
            return std::holds_alternative<ContainerType>(v);
        case PropertyGroup::ContainerContents:
            return std::holds_alternative<GroundContents>(v);
        default:
            return false;
    }
}
bool Type(ContextGroup g, const GroupValue& v) {
    switch (g) {
        case ContextGroup::LocalControl:
            return std::holds_alternative<LocalControl>(v);
        case ContextGroup::ViewPose:
            return std::holds_alternative<Camera>(v);
        case ContextGroup::ViewTarget:
            return std::holds_alternative<CameraTarget>(v);
        case ContextGroup::LocalWeapon:
            return std::holds_alternative<Weapon>(v);
        case ContextGroup::LocalSpectators:
            return std::holds_alternative<Spectators>(v);
        case ContextGroup::LocalControllerRotation:
            return std::holds_alternative<ControllerRotation>(v);
        case ContextGroup::LocalNativeHolding:
            return std::holds_alternative<Holding>(v);
        case ContextGroup::LocalNativeSway:
            return std::holds_alternative<Sway>(v);
        case ContextGroup::LocalAimBasis:
            return std::holds_alternative<AimBasis>(v);
        default:
            return false;
    }
}
bool FreshSource(const GroupSample& group, std::initializer_list<unsigned> path, const State& state) {
    const auto found = group.sources.find(std::vector<unsigned>(path));
    return found != group.sources.end() && HasValues(found->second.status) && !found->second.reused &&
           found->second.capture_sequence == state.capture_sequence &&
           found->second.source_category == state.category && ValidInterval(found->second.source, state.capture);
}
const GroupValue* LocalValue(const State& state, ContextGroup kind) {
    const auto found = state.context.local_groups.find(kind);
    return found != state.context.local_groups.end() && found->second.value ? &*found->second.value : nullptr;
}
template <class T> const T* LocalValue(const State& state, ContextGroup kind) {
    const auto* value = LocalValue(state, kind);
    return value ? std::get_if<T>(value) : nullptr;
}
bool ContextSources(const State& state) {
    const auto& c = state.context;
    const auto source = [&](std::initializer_list<unsigned> path, bool present) {
        if (!present)
            return true;
        const auto it = c.sources.find(std::vector<unsigned>(path));
        return it != c.sources.end() && HasValues(it->second.status) && it->second.capture_sequence &&
               ValidInterval(it->second.source, state.capture, it->second.reused);
    };
    return source({1}, c.world_token.has_value()) && source({2}, c.level_token.has_value()) &&
           source({8}, c.game_state_token.has_value()) && source({3}, c.map_name.has_value()) &&
           source({4}, !c.coordinate_convention.empty()) && source({5}, c.verified_meters_per_unit.has_value()) &&
           source({6}, c.origin.has_value()) && source({7, 1}, true) && source({7, 2}, true) && source({7, 3}, true) &&
           source({9}, true);
}
bool BusinessEvidence(const GroupSample& g, const State& s, const Entity* entity, std::set<u64>& members) {
    if (!g.value)
        return !HasValues(g.status);
    const auto owner = [&](const std::optional<Reference>& reference) {
        return reference && reference->kind == EntityKind::Character && (!entity || reference->id == entity->id);
    };
    const auto local_owner = [&](const std::optional<Reference>& reference) {
        const auto* control = LocalValue<LocalControl>(s, ContextGroup::LocalControl);
        return !reference || reference->kind == EntityKind::Character &&
                                 (!control || !control->character || reference == control->character);
    };
    return std::visit(
        [&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, Spatial>) {
                return (!v.position || v.origin.has_value()) &&
                       (!v.origin || !s.context.origin || v.origin == s.context.origin);
            } else if constexpr (std::is_same_v<T, Movement>) {
                if (v.component_velocity && !FreshSource(g, {1}, s))
                    return false;
                if (v.world) {
                    if (!FreshSource(g, {4}, s) || !v.world->velocity || !v.world->source || !owner(v.world->owner))
                        return false;
                }
                return true;
            } else if constexpr (std::is_same_v<T, Skeleton>) {
                if (!owner(v.owner) || !v.layout_token || !v.pose_token)
                    return false;
                for (unsigned index = 0; index < v.bones.size(); ++index) {
                    const auto& bone = v.bones[index];
                    if (bone.world_position && !FreshSource(g, {2, 0x80000000u | index, 5}, s))
                        return false;
                    const auto role = unsigned(bone.role) - 1;
                    const auto parent = BodyBoneParents[role];
                    if (parent < 0 ? bone.parent_role.has_value()
                                   : !bone.parent_role || unsigned(*bone.parent_role) != unsigned(parent + 1))
                        return false;
                    if (bone.world_position && !v.origin)
                        return false;
                }
                return !v.origin || !s.context.origin || v.origin == s.context.origin;
            } else if constexpr (std::is_same_v<T, GroundContents>) {
                for (size_t i = 0; i < v.members.size(); ++i) {
                    const auto& m = v.members[i];
                    if (s.entities.contains(m.id) || !members.insert(m.id).second ||
                        !FreshSource(g, {2, 0x80000000u | unsigned(i)}, s))
                        return false;
                    if (m.spatial && !FreshSource(g, {2, 0x80000000u | unsigned(i), 2}, s) ||
                        m.definition && !FreshSource(g, {2, 0x80000000u | unsigned(i), 3}, s) ||
                        m.stack && !FreshSource(g, {2, 0x80000000u | unsigned(i), 4}, s) ||
                        m.classification && !FreshSource(g, {2, 0x80000000u | unsigned(i), 5}, s))
                        return false;
                }
                return true;
            } else if constexpr (std::is_same_v<T, Seats>) {
                for (size_t i = 0; i < v.seats.size(); ++i) {
                    const auto& seat = v.seats[i];
                    for (size_t n = 0; n < seat.riders.size(); ++n)
                        if (!FreshSource(g, {4, 0x80000000u | unsigned(i), 4, 0x80000000u | unsigned(n)}, s))
                            return false;
                }
                return true;
            } else if constexpr (std::is_same_v<T, LocalControl>) {
                return (!v.character || v.character->kind == EntityKind::Character) &&
                       (!v.controlled || v.controlled->kind == EntityKind::Character ||
                        v.controlled->kind == EntityKind::Vehicle) &&
                       (!v.controller_token || FreshSource(g, {1}, s)) && (!v.character || FreshSource(g, {2}, s)) &&
                       (!v.controlled || FreshSource(g, {3}, s));
            } else if constexpr (std::is_same_v<T, Camera>) {
                const auto* control = LocalValue<LocalControl>(s, ContextGroup::LocalControl);
                return v.controller_token && v.source_token &&
                       (!control || !control->controller_token || v.controller_token == control->controller_token) &&
                       (!v.position || v.origin.has_value()) && (!v.position || FreshSource(g, {1}, s)) &&
                       (!v.rotation || FreshSource(g, {2}, s)) && (!v.fov_degrees || FreshSource(g, {3}, s)) &&
                       FreshSource(g, {8}, s) && FreshSource(g, {9}, s) &&
                       (!v.origin || !s.context.origin || v.origin == s.context.origin);
            } else if constexpr (std::is_same_v<T, Weapon>) {
                if (!local_owner(v.owner) || v.weapon_token && !FreshSource(g, {1}, s) ||
                    v.name && !FreshSource(g, {2}, s) || v.active_slot && !FreshSource(g, {3}, s) ||
                    v.has_current_item && !FreshSource(g, {7}, s) || v.owner && !FreshSource(g, {8}, s))
                    return false;
                if (v.has_current_item && *v.has_current_item &&
                    (!v.owner || !v.weapon_token || !v.active_slot || *v.active_slot < 0))
                    return false;
                return (!v.muzzle || FreshSource(g, {4}, s)) && (!v.recoil || FreshSource(g, {5}, s)) &&
                       (!v.trajectory_generation || FreshSource(g, {6}, s));
            } else if constexpr (std::is_same_v<T, Spectators>) {
                return (!v.count || v.owner.has_value()) && local_owner(v.owner) &&
                       (!v.count || FreshSource(g, {1}, s)) && (!v.owner || FreshSource(g, {2}, s));
            } else if constexpr (std::is_same_v<T, CameraTarget>) {
                return (!v.has_target || FreshSource(g, {1}, s)) && (!v.pending_active || FreshSource(g, {2}, s)) &&
                       (!v.target || FreshSource(g, {3}, s));
            } else if constexpr (std::is_same_v<T, Sway>) {
                return !v.rotation || FreshSource(g, {3}, s);
            } else if constexpr (std::is_same_v<T, ControllerRotation>) {
                const auto* control = LocalValue<LocalControl>(s, ContextGroup::LocalControl);
                const auto* camera = LocalValue<Camera>(s, ContextGroup::ViewPose);
                return (!v.rotation || v.controller_token.has_value()) && (!v.rotation || FreshSource(g, {2}, s)) &&
                       (!v.controller_token || FreshSource(g, {1}, s)) &&
                       (!control || !control->controller_token || v.controller_token == control->controller_token) &&
                       (!camera || !camera->controller_token || v.controller_token == camera->controller_token);
            } else if constexpr (std::is_same_v<T, Holding>) {
                return (!v.accepted_holding || FreshSource(g, {3}, s)) && (!v.blend || FreshSource(g, {4}, s));
            } else if constexpr (std::is_same_v<T, AimBasis>) {
                const auto* control = LocalValue<LocalControl>(s, ContextGroup::LocalControl);
                const auto* camera = LocalValue<Camera>(s, ContextGroup::ViewPose);
                const auto* weapon = LocalValue<Weapon>(s, ContextGroup::LocalWeapon);
                if (!local_owner(v.owner))
                    return false;
                if (v.direction && (!v.owner || !v.weapon_token || !v.controller_token || !v.native_context_token ||
                                    !v.native_profile || v.native_profile->empty() || !v.branch || !v.stage ||
                                    !v.native_zeroing_applied))
                    return false;
                if (v.branch == 1u &&
                    (!v.zeroing_distance_m || !v.zeroing_correction_degrees || v.native_zeroing_applied != true ||
                     v.native_profile != "2610.1.1.71/barrel-zeroing-v1"))
                    return false;
                if (v.branch == 2u &&
                    (v.native_profile != "2610.1.1.71/view-no-trace-v1" || v.native_zeroing_applied != false ||
                     v.zeroing_distance_m || v.zeroing_correction_degrees || v.integrated_steps ||
                     v.zeroing_reached_distance || v.integrated_distance_cm))
                    return false;
                if (v.owner && !FreshSource(g, {1}, s) || v.weapon_token && !FreshSource(g, {2}, s) ||
                    v.native_context_token && !FreshSource(g, {3}, s) || v.direction && !FreshSource(g, {6}, s) ||
                    v.zeroing_distance_m && !FreshSource(g, {7}, s) ||
                    v.zeroing_correction_degrees && !FreshSource(g, {8}, s) ||
                    v.integrated_steps && !FreshSource(g, {9}, s) ||
                    v.zeroing_reached_distance && !FreshSource(g, {10}, s) ||
                    v.integrated_distance_cm && !FreshSource(g, {11}, s) || v.branch && !FreshSource(g, {12}, s) ||
                    v.stage && !FreshSource(g, {13}, s) || v.native_zeroing_applied && !FreshSource(g, {14}, s) ||
                    v.native_profile && !FreshSource(g, {15}, s) || v.controller_token && !FreshSource(g, {16}, s))
                    return false;
                if (v.direction) {
                    const auto& d = *v.direction;
                    const auto norm = d.x * d.x + d.y * d.y + d.z * d.z;
                    if (std::abs(norm - 1.f) > .02f)
                        return false;
                }
                if ((v.integrated_steps.has_value() || v.zeroing_reached_distance.has_value() ||
                     v.integrated_distance_cm.has_value()) &&
                    !(v.integrated_steps && v.zeroing_reached_distance && v.integrated_distance_cm))
                    return false;
                return (!control || !control->controller_token || v.controller_token == control->controller_token) &&
                       (!camera || !camera->controller_token || v.controller_token == camera->controller_token) &&
                       (!weapon || !weapon->weapon_token || v.weapon_token == weapon->weapon_token) &&
                       (!weapon || !weapon->owner || v.owner == weapon->owner);
            } else
                return true;
        },
        *g.value);
}
bool Sample(const GroupSample& g, const State& s) {
    if (unsigned(g.status) > unsigned(FactStatus::NotImplemented))
        return false;
    if (HasValues(g.status) != g.value.has_value())
        return false;
    if (g.value) {
        if (!HasValues(g.status) || g.source_capture_sequence != s.capture_sequence ||
            !ValidInterval(g.source, s.capture) || !std::visit([](const auto& v) { return Valid(v); }, *g.value))
            return false;
    }
    for (const auto& [path, t] : g.sources) {
        if (path.empty() || path.size() > 12 || unsigned(t.source_category) > unsigned(DataCategory::Local) ||
            unsigned(t.status) > unsigned(FactStatus::NotImplemented))
            return false;
        if (HasValues(t.status) && (!t.capture_sequence || !ValidInterval(t.source, s.capture, t.reused) ||
                                    !t.reused && t.capture_sequence != s.capture_sequence))
            return false;
    }
    return true;
}
} // namespace
bool SourceTiming::operator==(const SourceTiming& b) const noexcept {
    return status == b.status && capture_sequence == b.capture_sequence && Interval(source, b.source) &&
           reused == b.reused && source_category == b.source_category;
}
bool GroupSample::operator==(const GroupSample& b) const {
    return status == b.status && source_capture_sequence == b.source_capture_sequence && Interval(source, b.source) &&
           value == b.value && sources == b.sources;
}
void Context::clear() noexcept {
    Context empty;
    swap(empty);
}
void Context::swap(Context& b) noexcept {
    using std::swap;
    swap(world_token, b.world_token);
    swap(level_token, b.level_token);
    swap(game_state_token, b.game_state_token);
    swap(native_world_velocity_supported, b.native_world_velocity_supported);
    map_name.swap(b.map_name);
    coordinate_convention.swap(b.coordinate_convention);
    swap(verified_meters_per_unit, b.verified_meters_per_unit);
    swap(origin, b.origin);
    swap(pacing, b.pacing);
    local_groups.swap(b.local_groups);
    sources.swap(b.sources);
}
bool State::SameEntity(u64 id, const PublicationState& other) const noexcept {
    const auto* typed = dynamic_cast<const State*>(&other);
    if (!typed)
        return false;
    const auto a = entities.find(id), b = typed->entities.find(id);
    return a != entities.end() && b != typed->entities.end() && a->second.kind == b->second.kind;
}
void State::clear() noexcept {
    State empty;
    swap(empty);
}
void State::swap(State& b) noexcept {
    using std::swap;
    swap(epoch, b.epoch);
    swap(revision, b.revision);
    swap(capture_sequence, b.capture_sequence);
    swap(accepted, b.accepted);
    swap(capture, b.capture);
    swap(read_finished, b.read_finished);
    swap(category, b.category);
    swap(features, b.features);
    context.swap(b.context);
    entities.swap(b.entities);
    removals.swap(b.removals);
}
bool HasValues(FactStatus s) noexcept {
    return s == FactStatus::Available || s == FactStatus::Partial;
}
bool GroupAllowed(EntityKind k, PropertyGroup g) noexcept {
    switch (k) {
        case EntityKind::Character:
            return g == PropertyGroup::Spatial || g == PropertyGroup::CharacterState ||
                   g == PropertyGroup::CharacterName || g == PropertyGroup::CharacterSkeleton ||
                   g == PropertyGroup::CharacterMovement;
        case EntityKind::Vehicle:
            return g == PropertyGroup::Spatial || g == PropertyGroup::VehicleState ||
                   g == PropertyGroup::VehicleSeats || g == PropertyGroup::VehicleName ||
                   g == PropertyGroup::VehicleModel || g == PropertyGroup::VehicleClassification;
        case EntityKind::Pickup:
            return g == PropertyGroup::Spatial || g == PropertyGroup::PickupStack ||
                   g == PropertyGroup::PickupDefinition || g == PropertyGroup::PickupItemClassification;
        case EntityKind::Creature:
            return g == PropertyGroup::Spatial || g == PropertyGroup::CreatureClassification;
        case EntityKind::Container:
            return g == PropertyGroup::ContainerClassification || g == PropertyGroup::ContainerContents;
        default:
            return false;
    }
}
bool LocalGroupAllowed(ContextGroup g) noexcept {
    return g == ContextGroup::LocalControl || g == ContextGroup::ViewPose || g == ContextGroup::ViewTarget ||
           g == ContextGroup::LocalWeapon || g == ContextGroup::LocalSpectators ||
           g == ContextGroup::LocalControllerRotation || g == ContextGroup::LocalNativeHolding ||
           g == ContextGroup::LocalNativeSway || g == ContextGroup::LocalAimBasis;
}
bool SameGroupValue(const GroupValue& a, const GroupValue& b) {
    return a == b;
}
bool SameState(const State& a, const State& b) {
    // removals只供本地delta原因选择，不属于基线当前业务状态。
    return a.epoch == b.epoch && a.revision == b.revision && a.capture_sequence == b.capture_sequence &&
           a.accepted == b.accepted && Interval(a.capture, b.capture) && a.read_finished == b.read_finished &&
           a.category == b.category && a.features == b.features && a.context == b.context && a.entities == b.entities;
}
bool ValidateState(const State& s, std::string& error) {
    const auto fail = [&](const char* why) {
        error = why;
        return false;
    };
    error.clear();
    if (!s.epoch || !s.revision || !s.capture_sequence || !ImplementedCategory(s.category) ||
        !ValidInterval(s.capture, s.capture))
        return fail("Invalid Consumer identity/category/capture");
    if (s.features)
        return fail("Unsupported Consumer feature bits");
    if (s.read_finished && (*s.read_finished < s.capture.started || *s.read_finished > s.capture.finished))
        return fail("Consumer read anchor outside batch");
    for (const auto& [id, why] : s.removals)
        if (!id || unsigned(why) > unsigned(RemovalReason::InstanceReplaced))
            return fail("Invalid Consumer removal");
    if (!s.accepted) {
        if (!s.entities.empty() || !s.context.local_groups.empty())
            return fail("Rejected Consumer state contains values");
        return true;
    }
    if (!std::isfinite(s.context.pacing.target_hz) || s.context.pacing.target_hz <= 0 ||
        s.context.pacing.target_hz > MaximumCategoryHz)
        return fail("Invalid Consumer pacing");
    if (!Valid(s.context.map_name) || !Valid(s.context.coordinate_convention) ||
        s.context.verified_meters_per_unit &&
            (!std::isfinite(*s.context.verified_meters_per_unit) || *s.context.verified_meters_per_unit <= 0))
        return fail("Invalid Consumer context");
    if (s.context.world_token && *s.context.world_token != s.epoch ||
        s.context.level_token && *s.context.level_token != s.epoch ||
        s.context.game_state_token && *s.context.game_state_token != s.epoch)
        return fail("Opaque context token contradicts epoch");
    if (s.category == DataCategory::Local && !s.entities.empty() ||
        s.category != DataCategory::Local && !s.context.local_groups.empty())
        return fail("Consumer groups belong to another category");
    for (const auto& [path, t] : s.context.sources)
        if (path.empty() || path.size() > 12 || unsigned(t.source_category) > unsigned(DataCategory::Local) ||
            unsigned(t.status) > unsigned(FactStatus::NotImplemented) ||
            HasValues(t.status) && (!t.capture_sequence || !ValidInterval(t.source, s.capture, t.reused) ||
                                    !t.reused && t.capture_sequence != s.capture_sequence))
            return fail("Invalid Consumer context source");
    for (const auto& [g, v] : s.context.local_groups)
        if (!LocalGroupAllowed(g) || !Sample(v, s) || v.value && !Type(g, *v.value))
            return fail("Invalid Consumer local group");
    for (const auto& [id, e] : s.entities) {
        if (!id || id != e.id || s.removals.contains(id))
            return fail("Invalid Consumer entity identity/removal");
        if (s.category == DataCategory::Living     ? e.kind != EntityKind::Character && e.kind != EntityKind::Creature
            : s.category == DataCategory::Vehicles ? e.kind != EntityKind::Vehicle
                                                   : e.kind != EntityKind::Pickup && e.kind != EntityKind::Container)
            return fail("Consumer entity belongs to another category");
        for (const auto& [g, v] : e.groups)
            if (!GroupAllowed(e.kind, g) || !Sample(v, s) || v.value && !Type(g, *v.value))
                return fail("Invalid Consumer entity group");
    }
    if (!ContextSources(s))
        return fail("Consumer context lacks its original source times");
    std::set<u64> member_ids;
    if (s.category == DataCategory::Local) {
        for (const auto group :
             {ContextGroup::LocalControl, ContextGroup::ViewPose, ContextGroup::ViewTarget, ContextGroup::LocalWeapon,
              ContextGroup::LocalSpectators, ContextGroup::LocalControllerRotation, ContextGroup::LocalNativeHolding,
              ContextGroup::LocalNativeSway, ContextGroup::LocalAimBasis})
            if (!s.context.local_groups.contains(group))
                return fail("Consumer Local is missing a terminal business group");
        for (const auto& [group, value] : s.context.local_groups)
            if (!BusinessEvidence(value, s, nullptr, member_ids))
                return fail("Consumer Local owner/token/source relationship is invalid");
    }
    for (const auto& [id, entity] : s.entities) {
        for (unsigned n = 0; n < unsigned(PropertyGroup::Count); ++n) {
            const auto group = PropertyGroup(n);
            if (consumer::GroupAllowed(entity.kind, group) && !entity.groups.contains(group))
                return fail("Consumer entity is missing a terminal business group");
        }
        for (const auto& [group, value] : entity.groups)
            if (!BusinessEvidence(value, s, &entity, member_ids))
                return fail("Consumer entity owner/topology/source relationship is invalid");
    }
    for (const auto& [id, why] : s.removals)
        if (!id || unsigned(why) > unsigned(RemovalReason::InstanceReplaced))
            return fail("Invalid Consumer removal");
    return true;
}
bool SameEpochContext(const Context& a, const Context& b, std::string& error) {
    error.clear();
    if (a.world_token && b.world_token && a.world_token != b.world_token ||
        a.level_token && b.level_token && a.level_token != b.level_token ||
        a.game_state_token && b.game_state_token && a.game_state_token != b.game_state_token ||
        a.origin && b.origin && a.origin != b.origin ||
        !a.coordinate_convention.empty() && !b.coordinate_convention.empty() &&
            a.coordinate_convention != b.coordinate_convention ||
        a.verified_meters_per_unit && b.verified_meters_per_unit &&
            a.verified_meters_per_unit != b.verified_meters_per_unit) {
        error = "Consumer world/level/coordinate identity changed within epoch";
        return false;
    }
    return true;
}
bool MergeEpochIdentity(const Context& a, const Context& b, Context& staged, std::string& error) {
    if (!SameEpochContext(a, b, error))
        return false;
    Context next;
    next.world_token = b.world_token ? b.world_token : a.world_token;
    next.level_token = b.level_token ? b.level_token : a.level_token;
    next.game_state_token = b.game_state_token ? b.game_state_token : a.game_state_token;
    next.origin = b.origin ? b.origin : a.origin;
    next.coordinate_convention = b.coordinate_convention.empty() ? a.coordinate_convention : b.coordinate_convention;
    next.verified_meters_per_unit =
        b.verified_meters_per_unit ? b.verified_meters_per_unit : a.verified_meters_per_unit;
    staged.swap(next);
    return true;
}
bool SameEpochContext(const State& s, const Context& c, std::string& e) {
    return SameEpochContext(s.context, c, e);
}
} // namespace monitor::synchronization::consumer
