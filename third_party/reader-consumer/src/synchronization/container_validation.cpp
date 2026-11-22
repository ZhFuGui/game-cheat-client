#include "synchronization/container_validation.hpp"
#include "diagnostics/work_progress.hpp"
#include <algorithm>
#include <limits>

namespace monitor::synchronization {
namespace {
using Object = Value::Object;
using List = Value::List;
using Path = std::vector<unsigned>;
constexpr unsigned Element = 0x80000000u;
const Object* ObjectOf(const Value& value) { return std::get_if<Object>(&value.data); }
const List* ListOf(const Value& value) { return std::get_if<List>(&value.data); }
bool Fields(const Object& object, std::initializer_list<unsigned> allowed) {
    return std::all_of(object.begin(), object.end(), [&](const auto& entry) {
        return std::find(allowed.begin(), allowed.end(), entry.first) != allowed.end();
    });
}
bool UInt(const Value& value) { return std::holds_alternative<u64>(value.data); }
bool Int(const Value& value) { return std::holds_alternative<std::int64_t>(value.data); }
bool Text(const Value& value) { return std::holds_alternative<std::string>(value.data); }
bool Real(const Value& value) { return std::holds_alternative<double>(value.data); }
bool UInt32(const Value& value) { return UInt(value) && std::get<u64>(value.data) <= UINT32_MAX; }
bool Int32(const Value& value) {
    return Int(value) && std::get<std::int64_t>(value.data) >= INT32_MIN &&
           std::get<std::int64_t>(value.data) <= INT32_MAX;
}
bool Vector(const Value& value, bool origin) {
    const auto* list = ListOf(value);
    return list && list->size() == 3 && std::all_of(list->begin(), list->end(), origin ? Int32 : Real);
}
bool Name(const Value& value) {
    const auto* object = ObjectOf(value);
    if (!object || !Fields(*object, {1, 2, 3, 4, 5})) return false;
    return std::all_of(object->begin(), object->end(), [](const auto& entry) {
        return entry.first == 5 ? Text(entry.second) : UInt32(entry.second);
    });
}
bool Instance(const Value& value) {
    const auto* object = ObjectOf(value);
    return object && object->size() == 3 && Fields(*object, {1, 2, 3}) && UInt(object->at(1)) &&
           std::get<u64>(object->at(1).data) && Int32(object->at(2)) &&
           std::get<std::int64_t>(object->at(2).data) >= 0 && UInt32(object->at(3));
}
bool ItemFields(unsigned group, const Value& value) {
    const auto* object = ObjectOf(value);
    if (!object) return false;
    const auto allowed = group == 1 ? std::initializer_list<unsigned>{1, 2} : group == 2
        ? std::initializer_list<unsigned>{1, 2, 3} : std::initializer_list<unsigned>{1, 2, 3, 4};
    if (!Fields(*object, allowed)) return false;
    for (const auto& [field, child] : *object) {
        if (group == 1 ? field == 1 ? !UInt(child) || !std::get<u64>(child.data) : !Int32(child)
            : group == 2 ? field == 1 ? !Int32(child) : !Real(child)
            : group == 3 ? field == 1 ? !UInt32(child) : field == 2 ? !Name(child) : field == 3 ? !Text(child) : !UInt(child)
            : field >= 3 ? !Text(child) : !UInt32(child)) return false;
    }
    return true;
}
bool Spatial(const Value& value) {
    const auto* object = ObjectOf(value);
    if (!object || !Fields(*object, {1, 2, 3, 4})) return false;
    for (const auto& [field, child] : *object)
        if (field <= 2 ? !Vector(child, field == 2) : !UInt(child)) return false;
    return true;
}
bool Member(const Value& value) {
    const auto* object = ObjectOf(value);
    if (!object || !Fields(*object, {1, 2, 3, 4, 5}) || !object->contains(1) || !object->contains(2) ||
        !object->contains(3) || !object->contains(4) || !Instance(object->at(1)) || !Spatial(object->at(4))) return false;
    const auto* association = ObjectOf(object->at(2));
    const auto* classification = ObjectOf(object->at(3));
    if (!association || association->size() != 3 || !Fields(*association, {1, 2, 3}) ||
        !UInt(association->at(1)) || !Int32(association->at(2)) || !UInt(association->at(3)) ||
        !classification || !Fields(*classification, {1, 2, 3})) return false;
    for (const auto& [field, child] : *classification)
        if (field == 3 ? !Text(child) : !UInt(child)) return false;
    if (!object->contains(5)) return true;
    const auto* item = ObjectOf(object->at(5));
    if (!item || !Fields(*item, {1, 2, 3, 4}) || !item->contains(1)) return false;
    for (const auto& [group, child] : *item)
        if (!ItemFields(group, child)) return false;
    return true;
}
bool Members(const Value& value) {
    const auto* object = ObjectOf(value);
    if (!object || object->size() != 3 || !Fields(*object, {1, 2, 3}) || !UInt32(object->at(2))) return false;
    const auto* members = ListOf(object->at(1));
    const auto* slots = ListOf(object->at(3));
    if (!members || !slots || std::get<u64>(object->at(2).data) != 1) return false;
    for (const auto& member : *members) if (!Member(member)) return false;
    for (const auto& record : *slots) {
        const auto* slot = ObjectOf(record);
        if (!slot || slot->size() != 3 || !Fields(*slot, {1, 2, 3}) || !Int32(slot->at(1)) ||
            !std::holds_alternative<bool>(slot->at(2).data) || !UInt32(slot->at(3)) ||
            std::get<u64>(slot->at(3).data) < 1 || std::get<u64>(slot->at(3).data) > 7) return false;
    }
    return true;
}
Path Child(Path path, unsigned field) { path.push_back(field); return path; }
bool Proven(const GroupState& group, const Path& path) {
    const auto found = group.nested_samples.find(path);
    return found != group.nested_samples.end() && found->second.value_status &&
           (found->second.latest_status == OwnedReadStatus::Available ||
            found->second.latest_status == OwnedReadStatus::Partial);
}
bool Source(const Value& value) {
    const auto* object = ObjectOf(value);
    if (!object || object->size() != 9 || !Fields(*object, {1, 2, 3, 4, 5, 6, 7, 8, 9})) return false;
    for (const auto& [field, child] : *object) if (field <= 3 ? !UInt(child) : !Int32(child)) return false;
    const auto number = [&](unsigned field) { return std::get<std::int64_t>(object->at(field).data); };
    const auto slots = number(4), capacity = number(5), bits = number(6), bitmap_capacity = number(7),
        first_free = number(8), free = number(9);
    return std::get<u64>(object->at(1).data) && slots >= 0 && capacity >= slots && bits == slots &&
           bitmap_capacity >= bits && free >= 0 && free <= slots && first_free >= -1 &&
           first_free < slots && (free ? first_free >= 0 : first_free == -1) &&
           (!slots || std::get<u64>(object->at(2).data)) && (!bits || std::get<u64>(object->at(3).data));
}
} // namespace

bool ContainerValueShape(PropertyGroup group, unsigned field, const Value& value, const State&, const EntityState&) {
    if (group == PropertyGroup::ContainerClassification)
        return field == 4 ? Name(value) : field == 3 ? Text(value) : UInt(value);
    if (group != PropertyGroup::ContainerContents) return false;
    if (field == 1) return Members(value);
    if (field >= 2 && field <= 4) return UInt32(value);
    return (field == 5 || field == 6) && Source(value);
}
bool ContainerGroupValid(PropertyGroup group, const GroupState& sample, const State& state, const EntityState& entity,
                         const std::set<u64>& actor_addresses, std::set<u64>& member_addresses) {
    if (group != PropertyGroup::ContainerClassification && group != PropertyGroup::ContainerContents) return true;
    if (sample.fields.empty()) return true;
    if (group == PropertyGroup::ContainerClassification)
        return sample.fields.contains(1) && std::get<u64>(sample.fields.at(1).data) == 1 &&
               sample.fields.contains(2) && std::get<u64>(sample.fields.at(2).data) && sample.fields.contains(3) &&
               !std::get<std::string>(sample.fields.at(3).data).empty();
    for (const auto& [path, observation] : sample.nested_samples)
        if (observation.value_status && observation.latest_status != OwnedReadStatus::Available &&
            observation.latest_status != OwnedReadStatus::Partial) return false;
    if (sample.fields.size() != 6 || !Proven(sample, {5}) || !Proven(sample, {6})) return false;
    const auto& opening = *ObjectOf(sample.fields.at(5));
    const auto& closing = *ObjectOf(sample.fields.at(6));
    if (opening.at(1) != closing.at(1)) return false;
    const auto declared = std::get<u64>(sample.fields.at(2).data), occupied = std::get<u64>(sample.fields.at(3).data),
        admitted = std::get<u64>(sample.fields.at(4).data);
    if (declared != u64(std::get<std::int64_t>(opening.at(4).data)) ||
        occupied != declared - u64(std::get<std::int64_t>(opening.at(9).data))) return false;
    const auto& collection = *ObjectOf(sample.fields.at(1));
    const auto& members = *ListOf(collection.at(1));
    const auto& slots = *ListOf(collection.at(3));
    if (slots.size() != declared || members.size() != admitted || admitted > occupied) return false;
    u64 allocated = 0, recorded_members = 0;
    for (std::size_t index = 0; index < slots.size(); ++index) {
        const auto& slot = *ObjectOf(slots[index]);
        const auto active = std::get<bool>(slot.at(2).data);
        const auto status = std::get<u64>(slot.at(3).data);
        if (std::get<std::int64_t>(slot.at(1).data) != std::int64_t(index) || active == (status == 1)) return false;
        allocated += active;
        recorded_members += status == 7;
        diagnostics::CompletedCpuUnit();
    }
    if (allocated != occupied || recorded_members != admitted) return false;
    std::set<std::int64_t> member_slots;
    for (unsigned index = 0; index < members.size(); ++index) {
        const auto& member = *ObjectOf(members[index]);
        const auto& identity = *ObjectOf(member.at(1));
        const auto& association = *ObjectOf(member.at(2));
        const auto address = std::get<u64>(identity.at(1).data);
        const auto slot_index = std::get<std::int64_t>(association.at(2).data);
        const Path path{1, 1, Element | index};
        if (!Proven(sample, path) || std::get<u64>(association.at(1).data) != entity.key.instance.actor_address ||
            std::get<u64>(association.at(3).data) != address || slot_index < 0 || u64(slot_index) >= declared ||
            !member_slots.insert(slot_index).second || actor_addresses.contains(address) ||
            !member_addresses.insert(address).second ||
            std::get<u64>(ObjectOf(slots[std::size_t(slot_index)])->at(3).data) != 7) return false;
        const auto& classification = *ObjectOf(member.at(3));
        if (classification.size() != 3 || !Proven(sample, Child(path, 3)) ||
            !std::get<u64>(classification.at(1).data) || !std::get<u64>(classification.at(2).data) ||
            std::get<std::string>(classification.at(3).data).empty()) return false;
        const auto& spatial = *ObjectOf(member.at(4));
        if (!spatial.empty() && !Proven(sample, Child(path, 4))) return false;
        for (unsigned field : {3u, 4u})
            if (spatial.contains(field) && std::get<u64>(spatial.at(field).data) != address) return false;
        if (spatial.contains(2)) {
            const auto coordinates = state.context.find(ContextGroup::Coordinates);
            if (coordinates == state.context.end() || !coordinates->second.fields.contains(3) ||
                spatial.at(2) != coordinates->second.fields.at(3)) return false;
        }
        diagnostics::CompletedCpuUnit();
        if (!member.contains(5)) continue;
        const auto& item = *ObjectOf(member.at(5));
        const auto& owner = *ObjectOf(item.at(1));
        if (!owner.empty() && (!owner.contains(1) || !Proven(sample, Child(path, 5)) ||
            std::get<u64>(owner.at(1).data) == address ||
            std::get<u64>(owner.at(1).data) == entity.key.instance.actor_address)) return false;
        if (owner.contains(2) && std::get<std::int64_t>(owner.at(2).data) != slot_index) return false;
        for (unsigned field : {2u, 3u, 4u})
            if (item.contains(field) && !ObjectOf(item.at(field))->empty() &&
                (owner.empty() || !Proven(sample, Child(Child(path, 5), field)))) return false;
    }
    return true;
}
std::set<Path> ContainerNestedPaths(const std::map<unsigned, Value>& fields) {
    std::set<Path> paths;
    for (unsigned field : {5u, 6u}) if (fields.contains(field)) paths.insert({field});
    const auto found = fields.find(1);
    if (found == fields.end()) return paths;
    const auto* collection = ObjectOf(found->second);
    if (!collection || !collection->contains(1)) return paths;
    const auto* members = ListOf(collection->at(1));
    if (!members) return paths;
    for (unsigned index = 0; index < members->size(); ++index) {
        const Path path{1, 1, Element | index};
        paths.insert(path);
        const auto* member = ObjectOf((*members)[index]);
        if (!member) continue;
        for (unsigned field : {3u, 4u}) if (member->contains(field)) paths.insert(Child(path, field));
        if (!member->contains(5)) continue;
        const auto item_path = Child(path, 5);
        paths.insert(item_path);
        const auto* item = ObjectOf(member->at(5));
        if (!item) continue;
        for (unsigned field : {2u, 3u, 4u}) if (item->contains(field)) paths.insert(Child(item_path, field));
    }
    return paths;
}
bool ContainerHasBusinessValue(const Path& path, const Value& value) {
    if (path.size() == 1 || path.size() == 3) return true; // 来源与已准入组件身份不能借父组续租。
    const auto* object = ObjectOf(value);
    if (!object) return true;
    if (path.size() == 4 && path.back() == 5)
        return object->contains(1) && ObjectOf(object->at(1)) && !ObjectOf(object->at(1))->empty();
    return !object->empty();
}
} // namespace monitor::synchronization
