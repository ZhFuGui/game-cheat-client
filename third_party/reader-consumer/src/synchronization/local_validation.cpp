#include "synchronization/local_validation.hpp"
#include "synchronization/motion_weapon_validation.hpp"
#include "memory/address.hpp"
#include <algorithm>
#include <cmath>

namespace monitor::synchronization {
namespace {
using Object = Value::Object;
using Path = std::vector<unsigned>;
const Object* ObjectFields(const Value& value, std::initializer_list<unsigned> allowed) {
    const auto* out = std::get_if<Object>(&value.data);
    if (!out)
        return nullptr;
    for (const auto& [key, field] : *out)
        if (std::find(allowed.begin(), allowed.end(), key) == allowed.end())
            return nullptr;
    return out;
}
bool UInt(const Value& value, u64 limit = UINT64_MAX) {
    const auto* number = std::get_if<u64>(&value.data);
    return number && *number <= limit;
}
bool Int(const Value& value, std::int64_t minimum, std::int64_t maximum) {
    const auto* number = std::get_if<std::int64_t>(&value.data);
    return number && *number >= minimum && *number <= maximum;
}
bool Identity(const Value& value) {
    const auto* object = ObjectFields(value, {1, 2, 3});
    return object && object->size() == 3 && object->contains(1) && object->contains(2) && object->contains(3) &&
           UInt(object->at(1)) && UserAddress(std::get<u64>(object->at(1).data)) && Int(object->at(2), 0, INT32_MAX) &&
           UInt(object->at(3), UINT32_MAX);
}
bool Name(const Value& value) {
    const auto* object = ObjectFields(value, {1, 2, 3, 4, 5});
    if (!object || object->size() != 5)
        return false;
    for (const auto field : {1u, 2u, 3u, 4u})
        if (!object->contains(field) || !UInt(object->at(field), UINT32_MAX))
            return false;
    return object->contains(5) && std::holds_alternative<std::string>(object->at(5).data) &&
           !std::get<std::string>(object->at(5).data).empty();
}
bool Weapon(const Value& value, const State& state) {
    const auto* object = ObjectFields(value, {1, 2, 3, 4, 5, 6});
    if (!object || object->empty())
        return object != nullptr;
    if (!object->contains(1) || !UInt(object->at(1)) || !object->contains(3) || !Identity(object->at(3)))
        return false;
    if (object->at(1) != std::get<Object>(object->at(3).data).at(1))
        return false;
    if (object->contains(5)) {
        const auto* targets = ObjectFields(object->at(5), {1, 2, 3});
        if (!targets || targets->contains(1) != targets->contains(2))
            return false;
        for (const auto& [key, field] : *targets) {
            const auto* number = std::get_if<double>(&field.data);
            if (!number || !std::isfinite(*number))
                return false;
        }
    }
    if (object->contains(6)) {
        const auto* generation = ObjectFields(object->at(6), {1});
        if (!generation || (generation->contains(1) && !UInt(generation->at(1), UINT32_MAX)))
            return false;
    }
    return (!object->contains(2) || Name(object->at(2))) &&
           (!object->contains(4) || MuzzleValueShape(object->at(4), state));
}
bool HasValues(const GroupState& group) {
    return group.status == FactStatus::Available || group.status == FactStatus::Partial;
}
bool Interval(const SampleInterval& interval, const State& state, const GroupState& group) {
    return interval.started != Clock::time_point{} && interval.started >= state.capture.started &&
           interval.started <= interval.finished && interval.finished <= group.verified_sample.finished &&
           interval.finished <= state.capture.finished;
}
bool Observation(const GroupState::NestedSample& observation, const Value& value, const State& state,
                 const GroupState& group) {
    const auto* object = std::get_if<Object>(&value.data);
    if (!object || observation.capture_sequence != state.capture_sequence ||
        !Interval(observation.attempt, state, group))
        return false;
    if (observation.value_status)
        return (*observation.value_status == OwnedReadStatus::Available ||
                *observation.value_status == OwnedReadStatus::Partial) &&
               observation.latest_status == *observation.value_status && !object->empty() &&
               Interval(observation.source, state, group) &&
               observation.source.started == observation.attempt.started &&
               observation.source.finished == observation.attempt.finished;
    return object->empty() && observation.latest_status == OwnedReadStatus::Failed &&
           observation.source.started == Clock::time_point{} && observation.source.finished == Clock::time_point{};
}
bool Owner(const Value& value, const State& state) {
    if (!Identity(value))
        return false;
    const auto& control = state.context.at(ContextGroup::LocalControl);
    const auto& identity = std::get<Object>(value.data);
    if (!HasValues(control) || !control.fields.contains(7) || control.fields.at(7) != identity.at(1))
        return false;
    if (control.fields.contains(11)) {
        const auto* reference = std::get_if<Object>(&control.fields.at(11).data);
        if (!reference || !reference->contains(5) || reference->at(5) != value)
            return false;
    }
    return true;
}
bool RealTriple(const Value& value) {
    const auto* angles = std::get_if<Value::List>(&value.data);
    return angles && angles->size() == 3 && std::all_of(angles->begin(), angles->end(), [](const Value& angle) {
               const auto* number = std::get_if<double>(&angle.data);
               return number && std::isfinite(*number);
           });
}
bool NativeCharacterAgrees(const Value& value, const State& state) {
    if (!Identity(value))
        return false;
    const auto& identity = std::get<Object>(value.data);
    const auto& control = state.context.at(ContextGroup::LocalControl).fields;
    if (control.contains(7) && control.at(7) != identity.at(1))
        return false;
    if (control.contains(11)) {
        const auto* reference = std::get_if<Object>(&control.at(11).data);
        if (!reference || !reference->contains(5) || reference->at(5) != value)
            return false;
    }
    for (const auto group : {ContextGroup::LocalWeapon, ContextGroup::LocalNativeHolding, ContextGroup::LocalNativeSway,
                             ContextGroup::LocalAimBasis}) {
        const auto found = state.context.find(group);
        if (found != state.context.end() && found->second.fields.contains(1) && found->second.fields.at(1) != value)
            return false;
    }
    return true;
}
bool NativeWeaponAgrees(const Value& value, const State& state) {
    if (!Identity(value))
        return false;
    for (const auto group : {ContextGroup::LocalNativeSway, ContextGroup::LocalAimBasis}) {
        const auto found = state.context.find(group);
        if (found != state.context.end() && found->second.fields.contains(2) && found->second.fields.at(2) != value)
            return false;
    }
    const auto& current = state.context.at(ContextGroup::LocalWeapon);
    if (!HasValues(current))
        return true; // 本组已经有独立核验；另一个读取组失败不能反向撤销它。
    if (!current.fields.contains(5))
        return false;
    const auto* weapon = std::get_if<Object>(&current.fields.at(5).data);
    if (weapon && weapon->contains(3) && weapon->at(3) != value)
        return false;
    const auto& identity = std::get<Object>(value.data);
    return !current.fields.contains(4) || current.fields.at(4) == identity.at(1);
}
} // namespace
bool LocalValueShape(ContextGroup group, unsigned field, const Value& value, const State& state) {
    if (group == ContextGroup::LocalControllerRotation) {
        if (field == 1)
            return Identity(value);
        return field == 2 && RealTriple(value);
    }
    if (group == ContextGroup::LocalNativeHolding) {
        if (field == 1 || field == 2)
            return Identity(value);
        if (field == 3)
            return std::holds_alternative<bool>(value.data);
        const auto* number = std::get_if<double>(&value.data);
        return field == 4 && number && std::isfinite(*number);
    }
    if (group == ContextGroup::LocalNativeSway)
        return field == 1 || field == 2 ? Identity(value) : field == 3 && RealTriple(value);
    if (group == ContextGroup::LocalAimBasis) {
        if (field <= 5)
            return Identity(value);
        if (field == 6) {
            if (!RealTriple(value))
                return false;
            double squared = 0;
            for (const auto& component : std::get<Value::List>(value.data)) {
                const auto number = std::get<double>(component.data);
                squared += number * number;
            }
            return std::isfinite(squared) && std::abs(squared - 1.0) <= 1e-3;
        }
        if (field == 9)
            return UInt(value, UINT32_MAX);
        if (field == 10)
            return std::holds_alternative<bool>(value.data);
        if (field == 12)
            return UInt(value) && (std::get<u64>(value.data) == 1 || std::get<u64>(value.data) == 2);
        if (field == 13)
            return UInt(value) && std::get<u64>(value.data) == 1;
        if (field == 14)
            return std::holds_alternative<bool>(value.data);
        if (field == 15) {
            const auto* profile = std::get_if<std::string>(&value.data);
            return profile && !profile->empty();
        }
        const auto* number = std::get_if<double>(&value.data);
        return (field == 7 || field == 8 || field == 11) && number && std::isfinite(*number);
    }
    if (group == ContextGroup::LocalSpectators)
        return field == 1 ? Owner(value, state) : field == 2 && UInt(value, INT32_MAX);
    if (group != ContextGroup::LocalWeapon)
        return false;
    if (field == 1)
        return Owner(value, state);
    if (field == 2)
        return Identity(value);
    if (field == 3)
        return Int(value, INT8_MIN, INT8_MAX);
    if (field == 4)
        return UInt(value);
    if (field == 5)
        return Weapon(value, state);
    if (field == 7)
        return std::holds_alternative<bool>(value.data);
    return field == 6 && UInt(value, INT32_MAX);
}
bool ValidateLocalBody(const State& state, std::string& error) {
    const auto fail = [&](const char* reason) {
        error = reason;
        return false;
    };
    if (const auto found = state.context.find(ContextGroup::LocalControllerRotation); found != state.context.end()) {
        const auto& sample = found->second;
        if (!sample.nested_samples.empty() || (HasValues(sample) && FieldMask(sample.fields) != 3))
            return fail("Controller rotation is missing its current source identity or angles");
        if (HasValues(sample)) {
            const auto& controller = std::get<Object>(sample.fields.at(1).data).at(1);
            const auto& control = state.context.at(ContextGroup::LocalControl).fields;
            const auto& camera = state.context.at(ContextGroup::ViewSource).fields;
            if ((control.contains(4) && control.at(4) != controller) ||
                (camera.contains(1) && camera.at(1) != controller))
                return fail("Controller rotation belongs to another local controller");
        }
    }
    if (const auto found = state.context.find(ContextGroup::LocalNativeHolding); found != state.context.end()) {
        const auto& sample = found->second;
        const auto mask = FieldMask(sample.fields);
        if (!sample.nested_samples.empty() ||
            (HasValues(sample) &&
             ((mask & 3) != 3 || !(mask & 12) || (sample.status == FactStatus::Available) != (mask == 15))))
            return fail("Native holding is missing its current owner/component or dynamic source");
        if (HasValues(sample) && !NativeCharacterAgrees(sample.fields.at(1), state))
            return fail("Native holding belongs to another current local body");
        for (const auto& [field, source] : sample.field_sources)
            if (source.origin != 1)
                return fail("Native holding must retain direct native field source semantics");
    }
    if (const auto found = state.context.find(ContextGroup::LocalNativeSway); found != state.context.end()) {
        const auto& sample = found->second;
        if (!sample.nested_samples.empty() || (HasValues(sample) && FieldMask(sample.fields) != 7))
            return fail("Native sway is missing its current body, weapon or computed rotation");
        if (HasValues(sample) &&
            (!NativeCharacterAgrees(sample.fields.at(1), state) || !NativeWeaponAgrees(sample.fields.at(2), state)))
            return fail("Native sway belongs to another current local body or weapon");
        for (const auto& [field, source] : sample.field_sources)
            if (source.origin != (field == 3 ? 2u : 1u))
                return fail("Native sway must distinguish read identities from computed native rotation");
    }
    if (const auto found = state.context.find(ContextGroup::LocalAimBasis); found != state.context.end()) {
        const auto& sample = found->second;
        constexpr u64 barrel_required = 0x78ff; // 五项身份、方向、归零两项和原生规则。
        constexpr u64 view_required = 0x782b;   // 身体、武器、GunData、方向和原生规则；未消费枪口及归零。
        constexpr u64 integration = 0x700; // 高级积分步数、到达状态和实际积分距离必须共同出现。
        const auto mask = FieldMask(sample.fields);
        if (!sample.nested_samples.empty())
            return fail("Native aim basis is missing required sources or has incomplete integration metadata");
        if (HasValues(sample)) {
            if (!sample.fields.contains(12) || !sample.fields.contains(14) || !sample.fields.contains(15))
                return fail("Native aim basis is missing its branch, zeroing decision or native profile");
            const auto branch = std::get_if<u64>(&sample.fields.at(12).data);
            const auto zeroed = std::get_if<bool>(&sample.fields.at(14).data);
            const auto profile = std::get_if<std::string>(&sample.fields.at(15).data);
            if (!branch || !zeroed || !profile)
                return fail("Native aim basis has invalid branch metadata");
            if (*branch == 1) {
                if ((mask != barrel_required && mask != (barrel_required | integration)) || !*zeroed ||
                    *profile != "2610.1.1.71/barrel-zeroing-v1")
                    return fail("Barrel aim basis requires exactly its native geometry and zeroing sources");
            } else if (*branch == 2) {
                if (mask != view_required || *zeroed || *profile != "2610.1.1.71/view-no-trace-v1")
                    return fail("View-no-trace aim basis cannot contain barrel or unproved trace data");
            } else {
                return fail("Native aim basis branch is not implemented");
            }
        }
        if (HasValues(sample) &&
            (!NativeCharacterAgrees(sample.fields.at(1), state) || !NativeWeaponAgrees(sample.fields.at(2), state)))
            return fail("Native aim basis belongs to another current local body or weapon");
        for (const auto& [field, source] : sample.field_sources)
            if (source.origin != (field <= 5 ? 1u : 2u))
                return fail("Native aim basis must distinguish read source identities from native interpretation");
    }
    const auto& spectators = state.context.at(ContextGroup::LocalSpectators);
    if (!spectators.nested_samples.empty() || (HasValues(spectators) && FieldMask(spectators.fields) != 3))
        return fail("Local spectator count is missing its current body or count");
    const auto& group = state.context.at(ContextGroup::LocalWeapon);
    if (!HasValues(group))
        return group.nested_samples.empty() || fail("Unavailable current weapon retains nested samples");
    const auto& fields = group.fields;
    if ((FieldMask(fields) & 39) != 39)
        return fail("Current weapon is missing current owner/processor/slot bounds");
    const auto active = std::get<std::int64_t>(fields.at(3).data);
    const auto count = std::get<u64>(fields.at(6).data);
    const bool selected = active >= 0 && u64(active) < count;
    if (fields.contains(4) != selected)
        return fail("Current weapon raw slot presence contradicts native bounds");
    const bool has_weapon = selected && std::get<u64>(fields.at(4).data) != 0;
    if (fields.contains(5) != has_weapon)
        return fail("Current weapon presence contradicts its selected raw slot");
    if (fields.contains(7)) {
        if (!group.field_sources.contains(7) || group.field_sources.at(7).origin != 2)
            return fail("Current item presence must retain its native selection interpretation source");
        const auto held = std::get_if<bool>(&fields.at(7).data);
        if (!held || *held != has_weapon)
            return fail("Current item presence contradicts its native selected slot");
        if (*held) {
            const auto* item = std::get_if<Object>(&fields.at(5).data);
            if (!item || !item->contains(3) || !Identity(item->at(3)))
                return fail("Current item presence has no verified object identity");
        }
    } else if (!has_weapon || group.status == FactStatus::Available) {
        return fail("Current item presence is missing despite a completed native selection");
    }
    std::map<Path, const Value*> nodes;
    if (has_weapon) {
        nodes.emplace(Path{5}, &fields.at(5));
        const auto& weapon = std::get<Object>(fields.at(5).data);
        for (const auto field : {4u, 5u, 6u})
            if (weapon.contains(field))
                nodes.emplace(Path{5, field}, &weapon.at(field));
    }
    if (nodes.size() != group.nested_samples.size())
        return fail("Current weapon nested evidence list is incomplete");
    for (const auto& [path, node] : nodes) {
        const auto found = group.nested_samples.find(path);
        if (found == group.nested_samples.end() || !Observation(found->second, *node, state, group))
            return fail("Current weapon or muzzle has no matching current independent observation");
        if (group.status == FactStatus::Available && found->second.latest_status != OwnedReadStatus::Available)
            return fail("Available current weapon hides a partial or failed nested task");
        if (path == Path{5, 5} && found->second.value_status == OwnedReadStatus::Available &&
            std::get<Object>(node->data).size() != 3)
            return fail("Available recoil targets are missing part of the native source");
        if (path == Path{5, 6} && found->second.value_status &&
            (found->second.value_status != OwnedReadStatus::Available || std::get<Object>(node->data).size() != 1))
            return fail("Native trajectory counter has an invalid value terminal");
    }
    return true;
}
} // namespace monitor::synchronization
