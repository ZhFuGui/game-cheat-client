#include "synchronization/container_projection.hpp"
#include "diagnostics/work_progress.hpp"
#include <stdexcept>
#include <type_traits>

namespace monitor::synchronization::detail {
namespace {
using Fields = Value::Object;
constexpr unsigned Element = 0x80000000u;
template <class T> void Put(Fields& out, unsigned field, const std::optional<T>& value) {
    if (!value)
        return;
    if constexpr (std::is_floating_point_v<T>)
        out.emplace(field, Value(double(*value)));
    else if constexpr (std::is_signed_v<T>)
        out.emplace(field, Value(std::int64_t(*value)));
    else
        out.emplace(field, Value(u64(*value)));
}
Value Name(const ObjectName& name) {
    Fields out;
    Put(out, 1, name.raw_index);
    Put(out, 2, name.raw_number);
    Put(out, 3, name.index);
    Put(out, 4, name.number);
    if (name.verified)
        out.emplace(5, Value(name.full));
    return Value(std::move(out));
}
Value Item(const OwnedItemEntry& item) {
    Fields out, owner;
    if (item.observation.value_status) {
        Put(owner, 1, item.source_object_address);
        Put(owner, 2, item.slot_index);
    }
    out.emplace(1, Value(std::move(owner)));
    if (item.stack) {
        Fields stack;
        if (item.stack->observation.value_status) {
            Put(stack, 1, item.stack->quantity);
            Put(stack, 2, item.stack->durability);
            Put(stack, 3, item.stack->max_durability);
        }
        out.emplace(2, Value(std::move(stack)));
    }
    if (item.item_info) {
        const auto& info = *item.item_info;
        Fields definition;
        if (info.observation.value_status) {
            Put(definition, 1, info.item_id);
            if (info.identifier)
                definition.emplace(2, Name(*info.identifier));
            if (info.display_name)
                definition.emplace(3, Value(*info.display_name));
            Put(definition, 4, info.definition_address);
        }
        out.emplace(3, Value(std::move(definition)));
        if (info.classification) {
            Fields classification;
            if (info.classification->value_status) {
                classification.emplace(1, Value(u64(info.category)));
                classification.emplace(2, Value(u64(info.classification_status)));
                classification.emplace(3, Value(info.classification_rule_id));
                classification.emplace(4, Value(info.classification_rules_revision));
            }
            out.emplace(4, Value(std::move(classification)));
        }
    }
    return Value(std::move(out));
}
Value Member(const GroundItemMember& member) {
    if (!member.object_internal_index || !member.object_serial_number || !member.observation.value_status)
        throw std::invalid_argument("Ground member lacks current lifecycle and ownership evidence");
    Fields out{{1, Value(Fields{{1, Value(member.component_address)},
                                {2, Value(std::int64_t(*member.object_internal_index))},
                                {3, Value(u64(*member.object_serial_number))}})},
               {2, Value(Fields{{1, Value(member.owner_actor)},
                                {2, Value(std::int64_t(member.slot_index))},
                                {3, Value(member.raw_component_value)}})}};
    Fields classification;
    if (member.classification.value_status) {
        Put(classification, 1, member.native_vtable);
        Put(classification, 2, member.class_address);
        classification.emplace(3, Value(member.classification_rule_id));
    }
    out.emplace(3, Value(std::move(classification)));
    Fields spatial;
    if (member.spatial.observation.value_status) {
        if (const auto p = member.spatial.position)
            spatial.emplace(1, Value(Value::List{Value(double(p->x)), Value(double(p->y)), Value(double(p->z))}));
        if (const auto o = member.spatial.origin)
            spatial.emplace(
                2, Value(Value::List{Value(std::int64_t(o->x)), Value(std::int64_t(o->y)), Value(std::int64_t(o->z))}));
        Put(spatial, 3, member.spatial.root_component_address);
        Put(spatial, 4, member.spatial.raw_root_component_value);
    }
    out.emplace(4, Value(std::move(spatial)));
    if (member.item)
        out.emplace(5, Item(*member.item));
    return Value(std::move(out));
}
Value Source(const GroundCollectionSource& source) {
    return Value(Fields{{1, Value(source.set_address)},
                        {2, Value(source.data_address)},
                        {3, Value(source.allocation_bitmap_address)},
                        {4, Value(std::int64_t(source.declared_slots))},
                        {5, Value(std::int64_t(source.capacity))},
                        {6, Value(std::int64_t(source.allocation_bits))},
                        {7, Value(std::int64_t(source.allocation_capacity))},
                        {8, Value(std::int64_t(source.first_free_index))},
                        {9, Value(std::int64_t(source.free_slots))}});
}
void Observe(std::map<std::vector<unsigned>, GroupState::NestedSample>& out, std::vector<unsigned> path,
             const OwnedObservation& observation) {
    GroupState::NestedSample sample;
    sample.capture_sequence = observation.capture_sequence;
    sample.attempt = {observation.started, observation.finished};
    sample.latest_status = observation.latest_status;
    sample.value_status = observation.value_status;
    if (observation.value_status)
        sample.source = sample.attempt;
    if (!out.emplace(std::move(path), std::move(sample)).second)
        throw std::invalid_argument("Duplicate ground member observation path");
}
std::vector<unsigned> Child(std::vector<unsigned> path, unsigned field) {
    path.push_back(field);
    return path;
}
} // namespace
std::map<unsigned, Value> ContainerContentsFields(const ContainerContentsSample& sample) {
    Value::List members, slots;
    members.reserve(sample.members.size());
    slots.reserve(sample.slots.size());
    for (const auto& member : sample.members) {
        members.emplace_back(Member(member));
        diagnostics::CompletedCpuUnit();
    }
    for (const auto& slot : sample.slots) {
        slots.emplace_back(Fields{
            {1, Value(std::int64_t(slot.slot_index))}, {2, Value(slot.allocated)}, {3, Value(u64(slot.status))}});
        diagnostics::CompletedCpuUnit();
    }
    Fields collection;
    collection.emplace(1, Value(std::move(members)));
    collection.emplace(2, Value(u64(sample.coverage)));
    collection.emplace(3, Value(std::move(slots)));
    Fields out;
    out.emplace(1, Value(std::move(collection)));
    Put(out, 2, sample.declared_slots);
    Put(out, 3, sample.occupied_slots);
    Put(out, 4, sample.admitted_members);
    if (sample.opening_source)
        out.emplace(5, Source(*sample.opening_source));
    if (sample.closing_source)
        out.emplace(6, Source(*sample.closing_source));
    return out;
}
std::map<std::vector<unsigned>, GroupState::NestedSample>
ContainerContentsSamples(const ContainerContentsSample& contents, const std::map<unsigned, Value>& fields) {
    std::map<std::vector<unsigned>, GroupState::NestedSample> out;
    if (fields.contains(5))
        Observe(out, {5}, contents.opening_observation);
    if (fields.contains(6))
        Observe(out, {6}, contents.closing_observation);
    if (!fields.contains(1))
        return out;
    for (unsigned i = 0; i < contents.members.size(); ++i) {
        const auto& member = contents.members[i];
        const std::vector<unsigned> path{1, 1, Element | i};
        Observe(out, path, member.observation);
        Observe(out, Child(path, 3), member.classification);
        Observe(out, Child(path, 4), member.spatial.observation);
        diagnostics::CompletedCpuUnit();
        if (!member.item)
            continue;
        const auto item_path = Child(path, 5);
        const auto& item = *member.item;
        Observe(out, item_path, item.observation);
        if (item.stack)
            Observe(out, Child(item_path, 2), item.stack->observation);
        if (item.item_info) {
            Observe(out, Child(item_path, 3), item.item_info->observation);
            if (item.item_info->classification)
                Observe(out, Child(item_path, 4), *item.item_info->classification);
        }
    }
    return out;
}
} // namespace monitor::synchronization::detail
