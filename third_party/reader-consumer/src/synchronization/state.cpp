#include "synchronization/state.hpp"
#include "diagnostics/work_progress.hpp"
#include "synchronization/container_validation.hpp"
#include "synchronization/skeleton_validation.hpp"
#include "synchronization/motion_weapon_validation.hpp"
#include "synchronization/local_validation.hpp"
#include "model/entities/field_presence.hpp"
#include "memory/address.hpp"
#include <cmath>
#include <algorithm>
#include <set>
#include <stdexcept>

namespace monitor::synchronization {
unsigned PropertyGroupToWire(PropertyGroup group) noexcept {
    switch (group) {
        case PropertyGroup::Identity:
            return 1;
        case PropertyGroup::Spatial:
            return 2;
        case PropertyGroup::CharacterState:
            return 3;
        case PropertyGroup::CharacterName:
            return 5;
        case PropertyGroup::VehicleState:
            return 6;
        case PropertyGroup::VehicleSeats:
            return 7;
        case PropertyGroup::VehicleName:
            return 8;
        case PropertyGroup::VehicleModel:
            return 9;
        case PropertyGroup::PickupOwner:
            return 10;
        case PropertyGroup::PickupStack:
            return 11;
        case PropertyGroup::PickupDefinition:
            return 12;
        case PropertyGroup::VehicleClassification:
            return 13;
        case PropertyGroup::PickupClassification:
            return 14;
        case PropertyGroup::CreatureClassification:
            return 15;
        case PropertyGroup::PickupItemClassification:
            return 16;
        case PropertyGroup::ContainerClassification:
            return 17;
        case PropertyGroup::ContainerContents:
            return 18;
        case PropertyGroup::CharacterSkeleton:
            return 20;
        case PropertyGroup::CharacterMovement:
            return 21;
        default:
            return 0;
    }
}
std::optional<PropertyGroup> PropertyGroupFromWire(unsigned number) noexcept {
    for (unsigned i = 0; i < unsigned(PropertyGroup::Count); ++i) {
        const auto group = static_cast<PropertyGroup>(i);
        if (PropertyGroupToWire(group) == number && number)
            return group;
    }
    return {};
}
unsigned FactStatusToWire(FactStatus status) noexcept {
    switch (status) {
        case FactStatus::Available:
            return 1;
        case FactStatus::Partial:
            return 2;
        case FactStatus::ReadFailed:
            return 3;
        case FactStatus::DependencyBlocked:
            return 4;
        case FactStatus::NotAttempted:
            return 5;
        case FactStatus::NotImplemented:
            return 6;
        default:
            return 0;
    }
}
std::optional<FactStatus> FactStatusFromWire(unsigned value) noexcept {
    for (unsigned i = 0; i <= unsigned(FactStatus::NotImplemented); ++i) {
        const auto status = static_cast<FactStatus>(i);
        if (value && FactStatusToWire(status) == value)
            return status;
    }
    return {};
}
GroupContract CanonicalContract() {
    GroupContract out;
    for (auto kind : {EntityKind::Character, EntityKind::Vehicle, EntityKind::Pickup, EntityKind::Creature,
                      EntityKind::Container}) {
        auto& entry = out[kind];
        for (unsigned i = 0; i < unsigned(PropertyGroup::Count); ++i) {
            const auto group = static_cast<PropertyGroup>(i);
            if (GroupAllowed(kind, group))
                entry.enabled.insert(group);
        }
    }
    return out;
}
bool ValidateContract(const GroupContract& contract, std::string& error) {
    error.clear();
    if (contract.empty()) {
        error = "No enabled entity kind";
        return false;
    }
    for (const auto& [kind, groups] : contract) {
        if (kind < EntityKind::Character || kind > EntityKind::Container ||
            !groups.enabled.contains(PropertyGroup::Identity)) {
            error = "Invalid kind or missing identity contract";
            return false;
        }
        for (auto group : groups.enabled)
            if (!GroupAllowed(kind, group)) {
                error = "Invalid enabled group for kind";
                return false;
            }
    }
    return true;
}
bool MakeContract(const std::vector<KindSelection>& selections, GroupContract& output, std::string& error) {
    GroupContract candidate;
    for (const auto& selection : selections) {
        if (candidate.contains(selection.kind)) {
            error = "Duplicate entity kind contract";
            return false;
        }
        KindContract groups;
        for (auto group : selection.enabled)
            if (!groups.enabled.insert(group).second) {
                error = "Duplicate enabled group";
                return false;
            }
        candidate.emplace(selection.kind, std::move(groups));
    }
    if (!ValidateContract(candidate, error))
        return false;
    output = std::move(candidate);
    return true;
}
u64 AllowedContextFields(ContextGroup group) noexcept {
    switch (group) {
        case ContextGroup::World:
            return 1023; // 八项源值及根未复验/缓存使用真实性声明。
        case ContextGroup::Map:
            return 127; // World、名称、根未核验、地图键、分类状态、规则、规则版本。
        case ContextGroup::Coordinates:
            return 15; // 坐标约定、经核验单位比例、原点、配置规则版本。
        case ContextGroup::LocalControl:
            return 4095; // 源关联、跨流ID及其本批身份引用。
        case ContextGroup::Pacing:
            return 7; // 配置版本、实际FPS、精确周期。
        case ContextGroup::ViewSource:
            return 15;
        case ContextGroup::ViewPose:
            return 127;
        case ContextGroup::ViewTarget:
            return 15;
        case ContextGroup::LocalWeapon:
            return 127;
        case ContextGroup::LocalSpectators:
            return 3;
        case ContextGroup::LocalControllerRotation:
            return 3;
        case ContextGroup::LocalNativeHolding:
            return 15;
        case ContextGroup::LocalNativeSway:
            return 7;
        case ContextGroup::LocalAimBasis:
            return 32767;
        default:
            return 0;
    }
}
u64 FieldMask(const std::map<unsigned, Value>& fields) {
    u64 mask = 0;
    for (const auto& [field, value] : fields) {
        if (!field || field > 64)
            throw std::invalid_argument("Property field outside mask domain");
        mask |= u64(1) << (field - 1);
    }
    return mask;
}
u64 AllowedFields(PropertyGroup group) noexcept {
    switch (group) {
        case PropertyGroup::Identity:
            return 7;
        case PropertyGroup::Spatial:
            return 15;
        case PropertyGroup::CharacterState:
            return 63;
        case PropertyGroup::CharacterName:
            return 1;
        case PropertyGroup::CharacterSkeleton:
            return 3;
        case PropertyGroup::CharacterMovement:
            return 15;
        case PropertyGroup::VehicleState:
            return 255;
        case PropertyGroup::VehicleSeats:
            return 7;
        case PropertyGroup::VehicleName:
            return 31;
        case PropertyGroup::VehicleModel:
            return 15;
        case PropertyGroup::PickupOwner:
            return 3;
        case PropertyGroup::PickupStack:
            return 7;
        case PropertyGroup::PickupDefinition:
            return 15;
        case PropertyGroup::VehicleClassification:
            return 7;
        case PropertyGroup::PickupClassification:
            return 7;
        case PropertyGroup::CreatureClassification:
            return 127;
        case PropertyGroup::PickupItemClassification:
            return 15;
        case PropertyGroup::ContainerClassification:
            return 15;
        case PropertyGroup::ContainerContents:
            return 63;
        default:
            return 0;
    }
}
bool GroupAllowed(EntityKind kind, PropertyGroup group) noexcept {
    if (group == PropertyGroup::Identity)
        return kind >= EntityKind::Character && kind <= EntityKind::Container;
    if (group == PropertyGroup::Spatial)
        return kind >= EntityKind::Character && kind <= EntityKind::Creature;
    switch (kind) {
        case EntityKind::Character:
            return (group >= PropertyGroup::CharacterState && group <= PropertyGroup::CharacterName) ||
                   group == PropertyGroup::CharacterSkeleton || group == PropertyGroup::CharacterMovement;
        case EntityKind::Vehicle:
            return group >= PropertyGroup::VehicleState && group <= PropertyGroup::VehicleModel ||
                   group == PropertyGroup::VehicleClassification;
        case EntityKind::Pickup:
            return group >= PropertyGroup::PickupOwner && group <= PropertyGroup::PickupDefinition ||
                   group == PropertyGroup::PickupClassification || group == PropertyGroup::PickupItemClassification;
        case EntityKind::Creature:
            return group == PropertyGroup::CreatureClassification;
        case EntityKind::Container:
            return group == PropertyGroup::ContainerClassification || group == PropertyGroup::ContainerContents;
        default:
            return false;
    }
}
namespace {
bool Unsigned(const Value& value) {
    return std::holds_alternative<u64>(value.data);
}
bool Signed(const Value& value) {
    return std::holds_alternative<std::int64_t>(value.data);
}
bool Text(const Value& value) {
    return std::holds_alternative<std::string>(value.data);
}
bool Boolean(const Value& value) {
    return std::holds_alternative<bool>(value.data);
}
bool Real(const Value& value) {
    return std::holds_alternative<double>(value.data);
}
bool Object(const Value& value) {
    return std::holds_alternative<Value::Object>(value.data);
}
bool Fields(const Value::Object& object, std::initializer_list<unsigned> allowed) {
    for (const auto& [field, value] : object)
        if (std::find(allowed.begin(), allowed.end(), field) == allowed.end())
            return false;
    return true;
}
bool ReferenceValue(const Value& value, const State& state, u64 expected_id = 0, u64 expected_address = 0) {
    const auto ref = std::get_if<Value::Object>(&value.data);
    if (!ref || ref->size() != 5 || !Fields(*ref, {1, 2, 3, 4, 5}) || !Text(ref->at(1)) ||
        std::get<std::string>(ref->at(1).data).empty() || !Unsigned(ref->at(2)) ||
        std::get<u64>(ref->at(2).data) != state.epoch || !Unsigned(ref->at(3)) || !std::get<u64>(ref->at(3).data) ||
        !Unsigned(ref->at(4)))
        return false;
    const auto id = std::get<u64>(ref->at(3).data), kind = std::get<u64>(ref->at(4).data);
    if ((expected_id && id != expected_id) || kind < 1 || kind > 5)
        return false;
    const auto identity = std::get_if<Value::Object>(&ref->at(5).data);
    if (!identity || identity->size() != 3 || !Fields(*identity, {1, 2, 3}) || !Unsigned(identity->at(1)) ||
        !std::get<u64>(identity->at(1).data) || !Signed(identity->at(2)) ||
        std::get<std::int64_t>(identity->at(2).data) < 0 || !Unsigned(identity->at(3)))
        return false;
    const auto address = std::get<u64>(identity->at(1).data);
    if (expected_address && address != expected_address)
        return false;
    if (auto found = state.entities.find(id); found != state.entities.end())
        return u64(found->second.key.kind) == kind && found->second.key.instance.actor_address == address &&
               found->second.key.instance.internal_index == std::get<std::int64_t>(identity->at(2).data) &&
               found->second.key.instance.serial_number == std::get<u64>(identity->at(3).data);
    return true; // 对应类别尚未到达：保留真实跨流身份，接收端稍后解析。
}
bool NameValue(const Value& value) {
    const auto object = std::get_if<Value::Object>(&value.data);
    if (!object || !Fields(*object, {1, 2, 3, 4, 5}))
        return false;
    for (const auto& [field, v] : *object)
        if (field == 5 ? !Text(v) : !Unsigned(v))
            return false;
    return true;
}
bool SeatsValue(const Value& value, const State& state) {
    const auto object = std::get_if<Value::Object>(&value.data);
    if (!object || object->size() != 2 || !object->contains(1) || !object->contains(2) || !Unsigned(object->at(2)))
        return false;
    const auto seats = std::get_if<Value::List>(&object->at(1).data);
    if (!seats)
        return false;
    for (const auto& entry : *seats) {
        const auto seat = std::get_if<Value::Object>(&entry.data);
        if (!seat || !Fields(*seat, {1, 2, 3, 4}) || seat->contains(3) && !Unsigned(seat->at(3)))
            return false;
        if (seat->contains(1) && !Signed(seat->at(1)) || seat->contains(2) && !Unsigned(seat->at(2)))
            return false;
        if (!seat->contains(4))
            continue;
        const auto riders = std::get_if<Value::Object>(&seat->at(4).data);
        if (!riders || !Fields(*riders, {1, 2, 3, 4}) || !riders->contains(3) || !riders->contains(4))
            return false;
        for (unsigned field : {1u, 2u, 3u})
            if (riders->contains(field) && !Unsigned(riders->at(field)))
                return false;
        const auto list = std::get_if<Value::List>(&riders->at(4).data);
        if (!list)
            return false;
        for (const auto& member : *list) {
            const auto rider = std::get_if<Value::Object>(&member.data);
            if (!rider || !Fields(*rider, {1, 2, 3, 4, 5}))
                return false;
            for (const auto& [field, v] : *rider)
                if (field == 5 ? !Object(v) : field == 3 ? !Signed(v) : !Unsigned(v))
                    return false;
            if (rider->contains(4)) {
                const auto id = std::get<u64>(rider->at(4).data);
                const auto character = state.entities.find(id);
                if (!id || !rider->contains(2) ||
                    (character != state.entities.end() &&
                     (character->second.key.kind != EntityKind::Character ||
                      std::get<u64>(rider->at(2).data) != character->second.key.instance.actor_address)) ||
                    (character == state.entities.end() &&
                     (!rider->contains(5) ||
                      !ReferenceValue(rider->at(5), state, id, std::get<u64>(rider->at(2).data)))))
                    return false;
            }
            if (rider->contains(5) && !ReferenceValue(rider->at(5), state))
                return false;
        }
    }
    return true;
}
bool Shape(PropertyGroup group, unsigned field, const Value& value, const State& state, const EntityState& entity) {
    switch (group) {
        case PropertyGroup::Identity: {
            if (field == 2)
                return Unsigned(value) && std::get<u64>(value.data) == u64(entity.key.kind);
            if (field == 3) {
                const auto list = std::get_if<Value::List>(&value.data);
                if (!list || list->empty())
                    return false;
                std::set<std::int64_t> slots;
                for (const auto& item : *list) {
                    const auto source = std::get_if<Value::Object>(&item.data);
                    if (!source || source->size() != 5 || !Fields(*source, {1, 2, 3, 4, 5}))
                        return false;
                    for (unsigned key : {1u, 2u, 3u, 5u})
                        if (!Unsigned(source->at(key)))
                            return false;
                    if (!Signed(source->at(4)) || std::get<u64>(source->at(1).data) != 1 ||
                        std::get<u64>(source->at(2).data) != state.source.opening_container ||
                        std::get<u64>(source->at(3).data) != state.source.level ||
                        std::get<u64>(source->at(5).data) != entity.key.instance.actor_address)
                        return false;
                    const auto slot = std::get<std::int64_t>(source->at(4).data);
                    if (slot < 0 || u64(slot) >= state.source.opening_slots || !slots.insert(slot).second)
                        return false;
                }
                return true;
            }
            const auto object = std::get_if<Value::Object>(&value.data);
            if (!object || object->size() != 3 || !object->contains(1) || !object->contains(2) || !object->contains(3))
                return false;
            if (field == 1)
                return Unsigned(object->at(1)) && Signed(object->at(2)) && Unsigned(object->at(3)) &&
                       std::get<u64>(object->at(1).data) == entity.key.instance.actor_address &&
                       std::get<std::int64_t>(object->at(2).data) == entity.key.instance.internal_index &&
                       std::get<u64>(object->at(3).data) == entity.key.instance.serial_number;
            return false;
        }
        case PropertyGroup::Spatial:
            if (field >= 3)
                return Unsigned(value);
            if (const auto values = std::get_if<Value::List>(&value.data); values && values->size() == 3) {
                for (const auto& component : *values)
                    if (field == 1 ? !Real(component) : !Signed(component))
                        return false;
                return true;
            }
            return false;
        case PropertyGroup::CharacterState:
            return field == 1 ? Signed(value) : field == 2 ? Boolean(value) : Unsigned(value);
        case PropertyGroup::CharacterName:
            return Text(value);
        case PropertyGroup::CharacterSkeleton:
            return SkeletonValueShape(field, value);
        case PropertyGroup::CharacterMovement:
            if (field == 4)
                return WorldVelocityValueShape(value, entity);
            if (field == 1) {
                const auto values = std::get_if<Value::List>(&value.data);
                if (!values || values->size() != 3)
                    return false;
                for (const auto& component : *values)
                    if (!Real(component) || !std::isfinite(std::get<double>(component.data)))
                        return false;
                return true;
            }
            if (field == 2) {
                const auto component = std::get_if<Value::Object>(&value.data);
                return component && component->size() == 3 && component->contains(1) && component->contains(2) &&
                       component->contains(3) && Unsigned(component->at(1)) && Signed(component->at(2)) &&
                       Unsigned(component->at(3)) && UserAddress(std::get<u64>(component->at(1).data)) &&
                       std::get<std::int64_t>(component->at(2).data) >= 0 &&
                       std::get<std::int64_t>(component->at(2).data) <= INT32_MAX &&
                       std::get<u64>(component->at(3).data) <= UINT32_MAX;
            }
            return field == 3 && Unsigned(value);
        case PropertyGroup::VehicleState:
            return field <= 4 ? Real(value) : field <= 6 ? Boolean(value) : Unsigned(value);
        case PropertyGroup::VehicleSeats:
            return field == 1 ? SeatsValue(value, state) : Unsigned(value);
        case PropertyGroup::VehicleName:
            return field == 5 ? Text(value) : Unsigned(value);
        case PropertyGroup::VehicleModel:
            return field == 3 ? NameValue(value) : field == 4 ? Text(value) : Unsigned(value);
        case PropertyGroup::PickupOwner:
            return field == 2 ? Signed(value) : Unsigned(value);
        case PropertyGroup::PickupStack:
            return field == 1 ? Signed(value) : Real(value);
        case PropertyGroup::PickupDefinition:
            return field == 2 ? NameValue(value) : field == 3 ? Text(value) : Unsigned(value);
        case PropertyGroup::VehicleClassification:
            return field == 3 ? Text(value) : Unsigned(value);
        case PropertyGroup::PickupClassification:
            return field == 3 ? NameValue(value) : field == 2 ? Text(value) : Unsigned(value);
        case PropertyGroup::CreatureClassification:
            return field == 7 ? NameValue(value) : field == 6 ? Text(value) : Unsigned(value);
        case PropertyGroup::PickupItemClassification:
            return field >= 3 ? Text(value) : Unsigned(value);
        case PropertyGroup::ContainerClassification:
        case PropertyGroup::ContainerContents:
            return ContainerValueShape(group, field, value, state, entity);
        default:
            return false;
    }
}
bool ValidValue(const Value& value, unsigned depth) {
    if (depth > 32)
        return false; // 当前属性类型只含有界层级，深层伪造结构不能进入模型。
    if (const auto n = std::get_if<double>(&value.data))
        return std::isfinite(*n);
    if (const auto list = std::get_if<Value::List>(&value.data))
        for (const auto& child : *list)
            if (!ValidValue(child, depth + 1))
                return false;
    if (const auto object = std::get_if<Value::Object>(&value.data))
        for (const auto& [field, child] : *object)
            if (!field || !ValidValue(child, depth + 1))
                return false;
    return true;
}
bool Interval(const SampleInterval& sample, const SampleInterval& capture) {
    return sample.started != Clock::time_point{} && sample.started >= capture.started &&
           sample.finished >= sample.started && sample.finished <= capture.finished;
}
using Path = std::vector<unsigned>;
constexpr unsigned Element = 0x80000000u;
Path Child(Path path, unsigned field) {
    path.push_back(field);
    return path;
}
std::set<Path> NestedPaths(PropertyGroup group, const std::map<unsigned, Value>& fields) {
    if (group == PropertyGroup::ContainerContents)
        return ContainerNestedPaths(fields);
    std::set<Path> paths;
    if (group == PropertyGroup::CharacterMovement) {
        if (fields.contains(4))
            paths.insert({4});
        return paths;
    }
    const auto first = fields.find(1);
    if (first == fields.end() || group != PropertyGroup::VehicleSeats)
        return paths;
    paths.insert({1});
    const auto object = std::get_if<Value::Object>(&first->second.data);
    if (!object || !object->contains(1))
        return paths;
    const auto list = std::get_if<Value::List>(&object->at(1).data);
    if (!list)
        return paths;
    for (unsigned i = 0; i < list->size(); ++i) {
        Path member{1, 1, Element | i};
        paths.insert(member);
        const auto value = std::get_if<Value::Object>(&(*list)[i].data);
        if (!value)
            continue;
        if (group == PropertyGroup::VehicleSeats && value->contains(4)) {
            const auto rider_path = Child(member, 4);
            paths.insert(rider_path);
            const auto riders = std::get_if<Value::Object>(&value->at(4).data);
            if (!riders || !riders->contains(4))
                continue;
            const auto entries = std::get_if<Value::List>(&riders->at(4).data);
            if (!entries)
                continue;
            for (unsigned n = 0; n < entries->size(); ++n) {
                auto path = Child(rider_path, 4);
                path.push_back(Element | n);
                paths.insert(std::move(path));
            }
        }
    }
    return paths;
}
const Value* PathValue(const std::map<unsigned, Value>& fields, const Path& path) {
    if (path.empty())
        return nullptr;
    const auto root = fields.find(path[0]);
    if (root == fields.end())
        return nullptr;
    const Value* value = &root->second;
    for (unsigned i = 1; i < path.size(); ++i) {
        if (path[i] & Element) {
            const auto list = std::get_if<Value::List>(&value->data);
            const auto index = path[i] & ~Element;
            if (!list || index >= list->size())
                return nullptr;
            value = &(*list)[index];
        } else {
            const auto object = std::get_if<Value::Object>(&value->data);
            if (!object || !object->contains(path[i]))
                return nullptr;
            value = &object->at(path[i]);
        }
    }
    return value;
}
bool HasBusinessValue(PropertyGroup group, const Path& path, const Value& value) {
    if (group == PropertyGroup::ContainerContents)
        return ContainerHasBusinessValue(path, value);
    const auto object = std::get_if<Value::Object>(&value.data);
    if (group == PropertyGroup::CharacterMovement && path == Path{4})
        return !object || !object->empty();
    if (path.size() == 1)
        return true; // 当前组声明的完整/部分集合本身需要原始源样本。
    if (!object)
        return true; // 分类标量等必须有自己的值证明。

    if (group == PropertyGroup::VehicleSeats) {
        if (path.size() == 3)
            return object->contains(3); // 占座状态是业务值，源索引/组件引用可保结构。
        if (path.size() == 4)
            return object->contains(1) || object->contains(2); // Riders 的声明计数。
        if (path.size() == 6)
            return object->contains(2) || object->contains(4); // 解密Pawn和已核验人物关联。
    }
    return !object->empty();
}
bool NestedEvidence(PropertyGroup group, const GroupState& sample, const State& state) {
    const auto paths = NestedPaths(group, sample.fields);
    if (paths.size() != sample.nested_samples.size())
        return false;
    for (const auto& path : paths) {
        const auto found = sample.nested_samples.find(path);
        if (found == sample.nested_samples.end())
            return false;
        const auto& evidence = found->second;
        const auto node = PathValue(sample.fields, path);
        if (!node)
            return false;
        if (unsigned(evidence.latest_status) < 1 || unsigned(evidence.latest_status) > 5)
            return false;
        if (evidence.latest_status == OwnedReadStatus::NotAttempted &&
            evidence.attempt.started == Clock::time_point{} && !evidence.value_status) {
            if (evidence.capture_sequence || evidence.attempt.finished != Clock::time_point{} ||
                evidence.source.started != Clock::time_point{} || evidence.source.finished != Clock::time_point{} ||
                HasBusinessValue(group, path, *node))
                return false;
            continue;
        }
        if (evidence.capture_sequence != state.capture_sequence || !Interval(evidence.attempt, state.capture))
            return false;
        if (evidence.value_status) {
            if ((*evidence.value_status != OwnedReadStatus::Available &&
                 *evidence.value_status != OwnedReadStatus::Partial) ||
                !Interval(evidence.source, state.capture) || evidence.source.finished > evidence.attempt.finished)
                return false;
        } else if (evidence.source.started != Clock::time_point{} || evidence.source.finished != Clock::time_point{} ||
                   HasBusinessValue(group, path, *node))
            return false;
    }
    return true;
}
bool ContextShape(ContextGroup group, unsigned field, const Value& value, const State& state) {
    switch (group) {
        case ContextGroup::World:
            return field >= 9   ? Boolean(value)
                   : field == 7 ? Real(value)
                   : field == 8 ? Signed(value)
                                : Unsigned(value) && (field != 6 || state.batch.category == DataCategory::Local ||
                                                      std::get<u64>(value.data) == state.source.level);
        case ContextGroup::Map:
            if (field == 2)
                return NameValue(value);
            if (field == 3)
                return Boolean(value);
            if (field == 4 || field == 6 || field == 7)
                return Text(value);
            return Unsigned(value);
        case ContextGroup::Coordinates:
            if (field == 1 || field == 4)
                return Text(value) && (field == 4 || !std::get<std::string>(value.data).empty());
            if (field == 2)
                return Real(value) && std::get<double>(value.data) > 0;
            if (const auto origin = std::get_if<Value::List>(&value.data); origin && origin->size() == 3) {
                return std::all_of(origin->begin(), origin->end(), Signed);
            }
            return false;
        case ContextGroup::LocalControl:
            if (field == 11 || field == 12) {
                const auto& local = state.context.at(ContextGroup::LocalControl).fields;
                const auto source = local.find(field == 11 ? 7u : 5u);
                const auto id = local.find(field == 11 ? 9u : 10u);
                return source != local.end() && id != local.end() && Unsigned(source->second) && Unsigned(id->second) &&
                       ReferenceValue(value, state, std::get<u64>(id->second.data), std::get<u64>(source->second.data));
            }
            if (field == 8) {
                const auto list = std::get_if<Value::List>(&value.data);
                return list && list->size() == 3 && std::all_of(list->begin(), list->end(), Real);
            }
            if (!Unsigned(value))
                return false;
            if (field <= 7)
                return true;
            if (const auto found = state.entities.find(std::get<u64>(value.data)); found != state.entities.end()) {
                const auto& local = state.context.at(ContextGroup::LocalControl).fields;
                const auto source = local.find(field == 9 ? 7u : 5u);
                return source != local.end() && Unsigned(source->second) &&
                       std::get<u64>(source->second.data) == found->second.key.instance.actor_address &&
                       (field == 9 ? found->second.key.kind == EntityKind::Character
                                   : found->second.key.kind == EntityKind::Character ||
                                         found->second.key.kind == EntityKind::Vehicle);
            }
            {
                const auto& local = state.context.at(ContextGroup::LocalControl).fields;
                const auto ref = local.find(field == 9 ? 11u : 12u);
                return ref != local.end() && ReferenceValue(ref->second, state, std::get<u64>(value.data));
            }
        case ContextGroup::Pacing:
            return field == 2 ? std::holds_alternative<double>(value.data) : Unsigned(value);
        case ContextGroup::LocalWeapon:
        case ContextGroup::LocalSpectators:
        case ContextGroup::LocalControllerRotation:
        case ContextGroup::LocalNativeHolding:
        case ContextGroup::LocalNativeSway:
        case ContextGroup::LocalAimBasis:
            return LocalValueShape(group, field, value, state);
        case ContextGroup::ViewSource:
        case ContextGroup::ViewTarget: {
            const bool identity = group == ContextGroup::ViewSource ? field == 2 || field == 4 : field == 2;
            if (!identity)
                return Unsigned(value) && (group == ContextGroup::ViewTarget || std::get<u64>(value.data));
            const auto instance = std::get_if<Value::Object>(&value.data);
            if (!instance || instance->size() != 3 || !Fields(*instance, {1, 2, 3}) || !Unsigned(instance->at(1)) ||
                !std::get<u64>(instance->at(1).data) || !Signed(instance->at(2)) ||
                std::get<std::int64_t>(instance->at(2).data) < 0 || !Unsigned(instance->at(3)) ||
                std::get<u64>(instance->at(3).data) > UINT32_MAX)
                return false;
            const auto& fields = state.context.at(group).fields;
            const auto address = fields.find(group == ContextGroup::ViewSource && field == 4 ? 3u : 1u);
            return address != fields.end() && Unsigned(address->second) &&
                   std::get<u64>(address->second.data) == std::get<u64>(instance->at(1).data);
        }
        case ContextGroup::ViewPose:
            if (field == 6 || field == 7)
                return Real(value);
            if (field == 5)
                return Real(value); // 源缓存时间可重复或处于初始化状态，不能据此建立年龄限制。
            if (field == 3)
                return Real(value) && std::get<double>(value.data) > 0 && std::get<double>(value.data) < 180;
            if (const auto list = std::get_if<Value::List>(&value.data); list && list->size() == 3)
                return std::all_of(list->begin(), list->end(), field == 4 ? Signed : Real);
            return false;
        default:
            return false;
    }
}
bool ValidateContext(const State& state, std::string& error) {
    const bool view = state.batch.category == DataCategory::Local;
    if (state.context.size() != ExpectedContextGroups(state.context, view)) {
        error = "Frame context terminal list is incomplete";
        return false;
    }
    for (const auto group : {ContextGroup::World, ContextGroup::Map, ContextGroup::Coordinates, ContextGroup::Pacing})
        if (!state.context.contains(group)) {
            error = "Missing required common context group";
            return false;
        }
    if (view)
        for (const auto& binding : LocalContextBindings)
            if (binding.required && !state.context.contains(binding.group)) {
                error = "Missing required typed Local context group";
                return false;
            }
    for (const auto& [group, sample] : state.context) {
        if (!AllowedContextFields(group) || (!view && IsLocalContextGroup(group))) {
            error = "Unknown or cross-category context group";
            return false;
        }
        const auto found = state.context.find(group);
        if (found == state.context.end()) {
            error = "Missing frame context";
            return false;
        }
        if (!FactStatusToWire(sample.status) || FieldMask(sample.fields) & ~AllowedContextFields(group) ||
            sample.verified_capture_sequence != state.capture_sequence ||
            !Interval(sample.verified_sample, state.capture) ||
            (group != ContextGroup::LocalWeapon && !sample.nested_samples.empty())) {
            error = "Invalid context proof or field domain";
            return false;
        }
        if (sample.status != FactStatus::Available && sample.status != FactStatus::Partial && !sample.fields.empty()) {
            error = "Unavailable context retains old values";
            return false;
        }
        if (sample.fields.size() != sample.field_sources.size()) {
            error = "Context original field sample list is incomplete";
            return false;
        }
        for (const auto& [field, value] : sample.fields) {
            const auto source = sample.field_sources.find(field);
            const bool upstream_map = source != sample.field_sources.end() && group == ContextGroup::Map &&
                                      field <= 3 &&
                                      source->second.capture_sequence == state.batch.directory_generation &&
                                      source->second.sample.started >= state.batch.directory_sample.started &&
                                      source->second.sample.finished <= state.capture.started;
            if (source == sample.field_sources.end() || !source->second.capture_sequence || source->second.origin < 1 ||
                source->second.origin > (IsLocalContextGroup(group) ? 2u : 3u) ||
                source->second.sample.started == Clock::time_point{} ||
                source->second.sample.finished < source->second.sample.started ||
                source->second.sample.finished > sample.verified_sample.finished ||
                !source->second.reused && (source->second.capture_sequence != state.capture_sequence ||
                                           source->second.sample.started < state.capture.started) ||
                source->second.reused && !(group == ContextGroup::World && field == 1 || upstream_map) ||
                (source->second.reused ? source->second.source_category != DataCategory::None
                                       : source->second.source_category != state.batch.category)) {
                error = "Invalid original context field sample";
                return false;
            }
        }
        for (const auto& [field, value] : sample.fields)
            if (!ValidValue(value, 0) || !ContextShape(group, field, value, state)) {
                error = "Invalid normalized frame context value";
                return false;
            }
    }
    const auto& world = state.context.at(ContextGroup::World);
    const u64 required_world = view ? 770 : 802; // 视图仅依赖World身份；不借Actor主Level名单授权。
    if ((world.status != FactStatus::Available && world.status != FactStatus::Partial) ||
        (FieldMask(world.fields) & required_world) != required_world || !std::get<u64>(world.fields.at(2).data)) {
        error = "Accepted context has no current World/Level proof";
        return false;
    }
    if (world.field_sources.contains(1) && world.field_sources.at(1).reused &&
        !std::get<bool>(world.fields.at(10).data)) {
        error = "Historical World root source contradicts cache-used fact";
        return false;
    }
    const auto& map = state.context.at(ContextGroup::Map);
    if (map.fields.contains(1) && std::get<u64>(map.fields.at(1).data) != std::get<u64>(world.fields.at(2).data)) {
        error = "Map sample belongs to a different World";
        return false;
    }
    const auto& pacing = state.context.at(ContextGroup::Pacing);
    if (pacing.status != FactStatus::Available || FieldMask(pacing.fields) != 7) {
        error = "Missing frozen pacing configuration";
        return false;
    }
    const auto fps = std::get<double>(pacing.fields.at(2).data);
    const auto interval = std::get<u64>(pacing.fields.at(3).data);
    if (!ValidCategoryHz(fps) || interval != PeriodForHz(fps)) {
        error = "Invalid exact frozen pacing period";
        return false;
    }
    if (view) {
        const auto& source = state.context.at(ContextGroup::ViewSource);
        const auto& control = state.context.at(ContextGroup::LocalControl);
        if ((source.status == FactStatus::Available || source.status == FactStatus::Partial) &&
            (source.status != FactStatus::Available || FieldMask(source.fields) != 15 ||
             (control.fields.contains(4) && source.fields.at(1) != control.fields.at(4)))) {
            error = "Camera source has no consistent verified controller/manager identity";
            return false;
        }
        if (source.status != FactStatus::Available && (!state.context.at(ContextGroup::ViewPose).fields.empty() ||
                                                       !state.context.at(ContextGroup::ViewTarget).fields.empty())) {
            error = "Unavailable camera source retains dependent pose or target values";
            return false;
        }
        if (!ValidateLocalBody(state, error))
            return false;
        const auto& pose = state.context.at(ContextGroup::ViewPose);
        if (pose.status == FactStatus::Available && (FieldMask(pose.fields) & 15) != 15) {
            error = "Available view pose is missing implemented current fields";
            return false;
        }
        if (pose.fields.contains(3)) {
            if (!pose.fields.contains(7)) {
                error = "Effective view FOV has no actual locked-FOV source";
                return false;
            }
            const auto locked = std::get<double>(pose.fields.at(7).data);
            if ((locked > 0 ? pose.fields.at(7) != pose.fields.at(3)
                            : !pose.fields.contains(6) || pose.fields.at(6) != pose.fields.at(3))) {
                error = "Effective view FOV contradicts its native selection sources";
                return false;
            }
        }
        if (pose.fields.contains(1) != pose.fields.contains(4)) {
            error = "View position and origin must have the same current presence";
            return false;
        }
        if (pose.fields.contains(4)) {
            const auto& coordinates = state.context.at(ContextGroup::Coordinates).fields;
            if (!coordinates.contains(3) || pose.fields.at(4) != coordinates.at(3)) {
                error = "View origin differs from current coordinate context";
                return false;
            }
        }
        const auto& target = state.context.at(ContextGroup::ViewTarget);
        if (target.fields.contains(1)) {
            if (!target.fields.contains(3) || !target.fields.contains(4)) {
                error = "Selected view target has no current and pending source associations";
                return false;
            }
            const auto pending = std::get<u64>(target.fields.at(4).data);
            if (target.fields.at(1) != target.fields.at(pending ? 4u : 3u)) {
                error = "Selected view target contradicts native pending/current selection";
                return false;
            }
        }
        if (target.fields.contains(2) && (!target.fields.contains(1) || !std::get<u64>(target.fields.at(1).data))) {
            error = "View target identity has no actual non-null association";
            return false;
        }
    }
    return true;
}
} // namespace
bool ValidateState(const State& state, std::string& error) {
    try {
        error.clear();
        const auto fail = [&](const char* why) {
            error = why;
            return false;
        };
        if (!state.epoch || !state.revision || !state.capture_sequence ||
            state.capture.started == Clock::time_point{} || state.capture.finished < state.capture.started)
            return fail("Invalid state epoch/revision/capture interval");
        if (state.read_finished &&
            (*state.read_finished < state.capture.started || *state.read_finished > state.capture.finished))
            return fail("Read-finished anchor is outside current capture");
        if (!state.accepted) {
            if (!state.entities.empty())
                return fail("Rejected state contains entities");
            for (const auto& [group, sample] : state.context)
                if (IsLocalContextGroup(group) && !sample.fields.empty())
                    return fail("Rejected view retains current values");
            return true;
        }
        const auto& batch = state.batch;
        const auto observed = [&](const SampleInterval& sample) {
            return sample.started != Clock::time_point{} && sample.started <= sample.finished &&
                   sample.finished <= state.capture.finished;
        };
        const bool view = batch.category == DataCategory::Local;
        if (!ImplementedCategory(batch.category) || batch.not_executed || batch.succeeded > batch.planned ||
            batch.read_failed > batch.planned - batch.succeeded ||
            batch.dependency_blocked != batch.planned - batch.succeeded - batch.read_failed)
            return fail("Invalid category batch generations, observation intervals or terminal task accounting");
        if (view) {
            const auto absent = [](const SampleInterval& interval) {
                return interval.started == Clock::time_point{} && interval.finished == Clock::time_point{};
            };
            if (batch.directory_generation || batch.classification_generation || batch.membership_generation ||
                !absent(batch.directory_sample) || !absent(batch.classification_sample) ||
                !absent(batch.membership_sample) || !state.entities.empty() || !state.removals.empty() ||
                !state.contract.empty() || state.source.opening_complete || state.source.closing_complete ||
                state.source.level || state.source.opening_slots || state.source.closing_slots ||
                state.source.opening_container || state.source.closing_container || !absent(state.source.opening) ||
                !absent(state.source.closing))
                return fail("Local must not claim Actor directory, source scope, contract or entity authority");
            return ValidateContext(state, error);
        }
        if (!batch.directory_generation || !batch.classification_generation || !batch.membership_generation ||
            !observed(batch.directory_sample) || !observed(batch.classification_sample) ||
            !observed(batch.membership_sample))
            return fail("Actor category is missing directory observations");
        if (!ValidateContract(state.contract, error))
            return false;
        if (!state.source.opening_complete || !state.source.closing_complete || !state.source.level ||
            !state.source.opening_container && state.source.opening_slots ||
            !state.source.closing_container && state.source.closing_slots ||
            state.source.opening.started == Clock::time_point{} ||
            state.source.opening.finished < state.source.opening.started ||
            state.source.opening.finished > state.capture.finished ||
            state.source.closing.started == Clock::time_point{} ||
            state.source.closing.finished < state.source.closing.started ||
            state.source.closing.finished > state.capture.finished)
            return fail("Accepted state has no complete current source boundaries");
        if (state.entities.size() > state.source.opening_slots || state.entities.size() > state.source.closing_slots)
            return fail("Entity count exceeds the verified source scope");
        if (!ValidateContext(state, error))
            return false;
        std::set<InstanceKey> keys;
        std::set<u64> addresses;
        std::set<u64> member_addresses;
        for (const auto& [id, entity] : state.entities) {
            if (!entity.key.instance.actor_address || !addresses.insert(entity.key.instance.actor_address).second)
                return fail("Duplicate or invalid instance identity");
            if (batch.category == DataCategory::Living
                    ? entity.key.kind != EntityKind::Character && entity.key.kind != EntityKind::Creature
                : batch.category == DataCategory::Vehicles
                    ? entity.key.kind != EntityKind::Vehicle
                    : entity.key.kind != EntityKind::Pickup && entity.key.kind != EntityKind::Container)
                return fail("Entity kind belongs to another category");
        }
        for (const auto& [id, entity] : state.entities) {
            if (!id || id != entity.id || entity.key.kind == EntityKind::Unknown ||
                !entity.key.instance.actor_address || entity.key.instance.internal_index < 0 ||
                !keys.insert(entity.key).second)
                return fail("Duplicate or invalid instance identity");
            const auto identity = entity.groups.find(PropertyGroup::Identity);
            if (identity == entity.groups.end() || identity->second.status != FactStatus::Available ||
                FieldMask(identity->second.fields) != AllowedFields(PropertyGroup::Identity))
                return fail("Instance has no current verified identity");
            const auto contract = state.contract.find(entity.key.kind);
            if (contract == state.contract.end() || entity.groups.size() != contract->second.enabled.size())
                return fail("Missing negotiated terminal property group");
            for (auto group : contract->second.enabled)
                if (!entity.groups.contains(group))
                    return fail("Missing negotiated terminal property group");
            for (const auto& [group, sample] : entity.groups) {
                if (!GroupAllowed(entity.key.kind, group) ||
                    static_cast<unsigned>(sample.status) > static_cast<unsigned>(FactStatus::NotImplemented))
                    return fail("Invalid property group or status");
                const auto fields = FieldMask(sample.fields);
                if (fields & ~AllowedFields(group))
                    return fail("Unknown property field");
                if (sample.status != FactStatus::Available && sample.status != FactStatus::Partial &&
                    !sample.fields.empty())
                    return fail("Unavailable property retains values");
                if (!Interval(sample.verified_sample, state.capture))
                    return fail("Property proof is outside current capture");
                if (sample.verified_capture_sequence != state.capture_sequence)
                    return fail("Property verification sequence does not match current capture");
                if (!sample.fields.empty() &&
                    (sample.source_sample.started == Clock::time_point{} ||
                     sample.source_sample.finished < sample.source_sample.started ||
                     sample.source_sample.finished > sample.verified_sample.finished ||
                     !sample.source_capture_sequence || sample.source_capture_sequence > state.capture_sequence))
                    return fail("Property source stamp is invalid");
                for (const auto& [field, value] : sample.fields)
                    if (!ValidValue(value, 0) || !Shape(group, field, value, state, entity))
                        return fail("Invalid property value or type");
                if (!NestedEvidence(group, sample, state))
                    return fail("Nested sample evidence is incomplete or invalid");
                if (group == PropertyGroup::CharacterState && sample.fields.contains(7)) {
                    const auto& local = state.context.at(ContextGroup::LocalControl).fields;
                    const auto owner = local.find(9);
                    if (owner == local.end() || !Unsigned(owner->second) || std::get<u64>(owner->second.data) != id ||
                        sample.source_capture_sequence != state.capture_sequence ||
                        sample.source_sample.started < state.capture.started)
                        return fail("Spectated count has no current local character ownership or source sample");
                }
                if (!ContainerGroupValid(group, sample, state, entity, addresses, member_addresses))
                    return fail("Container source, member or sampling proof is inconsistent");
                if (group == PropertyGroup::CharacterSkeleton && !SkeletonGroupValid(sample, state))
                    return fail("Skeleton source, bone coverage, coordinate basis or pose is inconsistent");
                if (!sample.field_sources.empty())
                    return fail("Entity group has unexpected context field samples");
                if (group == PropertyGroup::Spatial && sample.fields.contains(2)) {
                    const auto& coordinates = state.context.at(ContextGroup::Coordinates).fields;
                    if (!coordinates.contains(3) || coordinates.at(3) != sample.fields.at(2))
                        return fail("Spatial origin differs from current context");
                }
                diagnostics::CompletedCpuUnit();
            }
            diagnostics::CompletedCpuUnit();
        }
        return true;
    } catch (const std::invalid_argument& problem) {
        error = problem.what();
        return false;
    }
}
bool SameState(const State& a, const State& b) {
    const auto same = [](const SampleInterval& x, const SampleInterval& y) {
        return x.started == y.started && x.finished == y.finished;
    };
    const auto& left = a.batch;
    const auto& right = b.batch;
    if (left.category != right.category || left.directory_generation != right.directory_generation ||
        left.classification_generation != right.classification_generation ||
        left.membership_generation != right.membership_generation ||
        !same(left.directory_sample, right.directory_sample) ||
        !same(left.classification_sample, right.classification_sample) ||
        !same(left.membership_sample, right.membership_sample) || left.planned != right.planned ||
        left.succeeded != right.succeeded || left.read_failed != right.read_failed ||
        left.dependency_blocked != right.dependency_blocked || left.not_executed != right.not_executed)
        return false;
    if (a.epoch != b.epoch || a.revision != b.revision || a.capture_sequence != b.capture_sequence ||
        a.accepted != b.accepted || !same(a.capture, b.capture) || a.read_finished != b.read_finished ||
        a.removals != b.removals || a.entities.size() != b.entities.size() || a.contract != b.contract ||
        a.context.size() != b.context.size())
        return false;
    const auto same_group = [&](const GroupState& x, const GroupState& y) {
        if (x.status != y.status || x.fields != y.fields || !same(x.source_sample, y.source_sample) ||
            !same(x.verified_sample, y.verified_sample) || x.source_capture_sequence != y.source_capture_sequence ||
            x.verified_capture_sequence != y.verified_capture_sequence ||
            x.nested_samples.size() != y.nested_samples.size() || x.field_sources.size() != y.field_sources.size())
            return false;
        for (const auto& [field, source] : x.field_sources) {
            const auto found = y.field_sources.find(field);
            if (found == y.field_sources.end())
                return false;
            const auto& other = found->second;
            if (source.capture_sequence != other.capture_sequence || source.reused != other.reused ||
                source.source_category != other.source_category || source.origin != other.origin ||
                !same(source.sample, other.sample))
                return false;
        }
        for (const auto& [path, sample] : x.nested_samples) {
            const auto found = y.nested_samples.find(path);
            if (found == y.nested_samples.end())
                return false;
            const auto& other = found->second;
            if (sample.capture_sequence != other.capture_sequence || sample.latest_status != other.latest_status ||
                sample.value_status != other.value_status || !same(sample.source, other.source) ||
                !same(sample.attempt, other.attempt))
                return false;
        }
        return true;
    };
    for (const auto& [group, sample] : a.context) {
        const auto found = b.context.find(group);
        if (found == b.context.end() || !same_group(sample, found->second))
            return false;
    }
    const auto& x = a.source;
    const auto& y = b.source;
    if (x.opening_complete != y.opening_complete || x.closing_complete != y.closing_complete || x.level != y.level ||
        x.opening_slots != y.opening_slots || x.closing_slots != y.closing_slots ||
        x.opening_container != y.opening_container || x.closing_container != y.closing_container ||
        !same(x.opening, y.opening) || !same(x.closing, y.closing))
        return false;
    for (const auto& [id, entity] : a.entities) {
        const auto found = b.entities.find(id);
        if (found == b.entities.end() || entity.id != found->second.id || entity.key != found->second.key ||
            entity.groups.size() != found->second.groups.size())
            return false;
        for (const auto& [group, state] : entity.groups) {
            const auto other = found->second.groups.find(group);
            if (other == found->second.groups.end() || !same_group(state, other->second))
                return false;
        }
    }
    return true;
}
bool SameEpochContext(const FrameContext& confirmed, const FrameContext& current, std::string& error) {
    for (const auto kind : {ContextGroup::World, ContextGroup::Coordinates}) {
        const auto before = confirmed.find(kind), after = current.find(kind);
        if (before == confirmed.end() || after == current.end())
            continue;
        for (const auto field : {1u, 2u, 3u, 4u, 6u}) {
            if (kind == ContextGroup::World ? field != 2 && field != 6 : field > 4)
                continue;
            const auto old = before->second.fields.find(field), value = after->second.fields.find(field);
            if (old != before->second.fields.end() && value != after->second.fields.end() &&
                old->second != value->second) {
                error = "World/Level/coordinate identity changed without a new epoch baseline";
                return false;
            }
        }
    }
    return true;
}
bool MergeEpochIdentity(const FrameContext& confirmed, const FrameContext& current, FrameContext& staged,
                        std::string& error) {
    if (!SameEpochContext(confirmed, current, error))
        return false;
    FrameContext result;
    for (const auto* context : {&confirmed, &current}) {
        for (const auto kind : {ContextGroup::World, ContextGroup::Coordinates}) {
            const auto group = context->find(kind);
            if (group == context->end())
                continue;
            for (const auto field : {1u, 2u, 3u, 4u, 6u}) {
                if (kind == ContextGroup::World ? field != 2 && field != 6 : field > 4)
                    continue;
                const auto value = group->second.fields.find(field);
                if (value != group->second.fields.end())
                    result[kind].fields.try_emplace(field, value->second);
            }
        }
    }
    staged = std::move(result);
    return true;
}
bool SameEpochContext(const State& baseline, const FrameContext& current, std::string& error) {
    return SameEpochContext(baseline.context, current, error);
}
} // namespace monitor::synchronization
