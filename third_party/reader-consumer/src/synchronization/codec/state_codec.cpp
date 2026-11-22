#include "synchronization/codec/detail.hpp"

namespace monitor::synchronization::codec::detail {
namespace {
bool SameNested(const std::map<std::vector<unsigned>, GroupState::NestedSample>& first,
                const std::map<std::vector<unsigned>, GroupState::NestedSample>& second) {
    if (first.size() != second.size())
        return false;
    for (const auto& [path, a] : first) {
        const auto found = second.find(path);
        if (found == second.end())
            return false;
        const auto& b = found->second;
        if (a.capture_sequence != b.capture_sequence || a.latest_status != b.latest_status ||
            a.value_status != b.value_status || a.source.started != b.source.started ||
            a.source.finished != b.source.finished || a.attempt.started != b.attempt.started ||
            a.attempt.finished != b.attempt.finished)
            return false;
    }
    return true;
}
Value Vector(const Value& value, bool to_wire) {
    if (to_wire) {
        const auto* list = std::get_if<List>(&value.data);
        Require(list && list->size() == 3, "Invalid coordinate vector");
        Object out;
        for (unsigned i = 0; i < 3; ++i)
            out.emplace(i + 1, (*list)[i]);
        return Value(std::move(out));
    }
    const auto* object = std::get_if<Object>(&value.data);
    Require(object && object->size() == 3 && object->contains(1) && object->contains(2) && object->contains(3),
            "Invalid coordinate message");
    return Value(List{object->at(1), object->at(2), object->at(3)});
}
Object Source(const SourceScope& s, u64 opening_sequence, u64 closing_sequence, const Binding& b) {
    Object out;
    Put(out, 1, s.opening_complete);
    Put(out, 2, s.closing_complete);
    Put(out, 3, s.level);
    Put(out, 4, s.opening_slots);
    Put(out, 5, s.closing_slots);
    if (s.opening.started != Clock::time_point{})
        Put(out, 6, Interval(s.opening, opening_sequence, b));
    if (s.closing.started != Clock::time_point{})
        Put(out, 7, Interval(s.closing, closing_sequence, b));
    Put(out, 8, s.opening_container);
    Put(out, 9, s.closing_container);
    return out;
}
SourceScope ReadSource(const Object& object, u64 opening_sequence, u64 closing_sequence, const Binding& b) {
    SourceScope out;
    out.opening_complete = Boolean(object, 1);
    out.closing_complete = Boolean(object, 2);
    out.level = UInt(object, 3);
    out.opening_slots = UInt(object, 4);
    out.closing_slots = UInt(object, 5);
    u64 sample = 0;
    if (object.contains(6)) {
        out.opening = ReadInterval(Message(object, 6), b, sample);
        Require(sample == opening_sequence, "Source opening directory generation mismatch");
    }
    if (object.contains(7)) {
        out.closing = ReadInterval(Message(object, 7), b, sample);
        Require(sample == closing_sequence, "Source closing membership generation mismatch");
    }
    out.opening_container = UInt(object, 8);
    out.closing_container = UInt(object, 9);
    return out;
}
Object Proof(PropertyGroup group, const GroupProof& proof, const Binding& b) {
    Require(PropertyGroupToWire(group) && FactStatusToWire(proof.status) &&
                !(proof.present_fields & ~AllowedFields(group)),
            "Invalid property proof domain");
    Require(proof.status == FactStatus::Available || proof.status == FactStatus::Partial || !proof.present_fields,
            "Unavailable proof has current values");
    Object out;
    Put(out, 1, u64(PropertyGroupToWire(group)));
    Put(out, 2, u64(FactStatusToWire(proof.status)));
    Put(out, 3, proof.present_fields);
    Put(out, 4, Interval(proof.verified_sample, proof.verified_capture_sequence, b));
    Put(out, 5, NestedList(proof.nested_samples, b));
    if (proof.present_fields)
        Put(out, 6, Interval(proof.source_sample, proof.source_capture_sequence, b));
    return out;
}
GroupProof ReadProof(const Object& object, PropertyGroup& group, const Binding& b) {
    const auto g = PropertyGroupFromWire(unsigned(UInt(object, 1)));
    const auto s = FactStatusFromWire(unsigned(UInt(object, 2)));
    Require(g && s, "Invalid proof group/status");
    group = *g;
    GroupProof out;
    out.status = *s;
    out.present_fields = UInt(object, 3);
    Require(!(out.present_fields & ~AllowedFields(group)), "Proof field mask outside group");
    out.verified_sample = ReadInterval(Message(object, 4), b, out.verified_capture_sequence);
    out.nested_samples = ReadNested(Repeated(object, 5), b);
    Require(object.contains(6) == bool(out.present_fields), "Property source presence differs from current fields");
    if (object.contains(6))
        out.source_sample = ReadInterval(Message(object, 6), b, out.source_capture_sequence);
    return out;
}
Object EntityProofValue(u64 id, const EntityProof& proof, const Binding& b) {
    const auto contract = b.contract.find(proof.key.kind);
    Require(id && contract != b.contract.end() && proof.groups.size() == contract->second.enabled.size(),
            "Invalid/incomplete entity proof contract");
    for (auto group : contract->second.enabled)
        Require(proof.groups.contains(group), "Proof omitted negotiated terminal group");
    Object out;
    Put(out, 1, id);
    Put(out, 2, u64(proof.key.kind));
    Put(out, 3, Instance(proof.key.instance));
    List groups;
    for (const auto& [g, p] : proof.groups)
        groups.emplace_back(Proof(g, p, b));
    Put(out, 4, std::move(groups));
    return out;
}
EntityProof ReadEntityProof(const Object& object, u64& id, const Binding& b) {
    id = UInt(object, 1);
    Require(id, "Missing proof entity ID");
    EntityProof out;
    out.key = {EntityKind(UInt(object, 2)), ReadInstance(Message(object, 3))};
    Require(unsigned(out.key.kind) >= 1 && unsigned(out.key.kind) <= unsigned(EntityKind::Container),
            "Unknown entity kind");
    for (const auto& value : Repeated(object, 4)) {
        PropertyGroup group;
        auto proof = ReadProof(std::get<Object>(value.data), group, b);
        Require(GroupAllowed(out.key.kind, group) && out.groups.emplace(group, std::move(proof)).second,
                "Duplicate/invalid proof group");
    }
    const auto contract = b.contract.find(out.key.kind);
    Require(contract != b.contract.end() && out.groups.size() == contract->second.enabled.size(),
            "Proof omitted negotiated terminal group");
    for (auto group : contract->second.enabled)
        Require(out.groups.contains(group), "Proof omitted negotiated terminal group");
    return out;
}
Object Property(PropertyGroup group, const GroupState& sample, const Binding& b) {
    Require(PropertyGroupToWire(group) && FactStatusToWire(sample.status) &&
                !(FieldMask(sample.fields) & ~AllowedFields(group)),
            "Invalid property state domain");
    Require(sample.status == FactStatus::Available || sample.status == FactStatus::Partial || sample.fields.empty(),
            "Unavailable property has current values");
    Object out;
    Put(out, 1, u64(PropertyGroupToWire(group)));
    Put(out, 2, u64(FactStatusToWire(sample.status)));
    Put(out, 3, FieldMask(sample.fields));
    if (!sample.fields.empty()) {
        Object wrapper;
        wrapper.emplace(PropertyGroupToWire(group), Value(PropertyFields(group, sample.fields, true)));
        Put(out, 4, std::move(wrapper));
    }
    if (sample.source_sample.started != Clock::time_point{})
        Put(out, 5, Interval(sample.source_sample, sample.source_capture_sequence, b));
    Put(out, 6, Interval(sample.verified_sample, sample.verified_capture_sequence, b));
    Put(out, 7, NestedList(sample.nested_samples, b));
    return out;
}
GroupState ReadProperty(const Object& object, PropertyGroup& group, const Binding& b) {
    const auto g = PropertyGroupFromWire(unsigned(UInt(object, 1)));
    const auto s = FactStatusFromWire(unsigned(UInt(object, 2)));
    Require(g && s, "Unknown property group/status");
    group = *g;
    GroupState out;
    out.status = *s;
    if (object.contains(4)) {
        const auto& wrapper = Message(object, 4);
        Require(wrapper.size() == 1 && wrapper.contains(PropertyGroupToWire(group)),
                "Property value oneof does not match group");
        out.fields = PropertyFields(
            group, DeclaredPropertyFields(group, Message(wrapper, PropertyGroupToWire(group)), UInt(object, 3)), false);
    }
    Require(UInt(object, 3) == FieldMask(out.fields) && !(UInt(object, 3) & ~AllowedFields(group)),
            "Property presence mask mismatch");
    if (object.contains(5))
        out.source_sample = ReadInterval(Message(object, 5), b, out.source_capture_sequence);
    out.verified_sample = ReadInterval(Message(object, 6), b, out.verified_capture_sequence);
    out.nested_samples = ReadNested(Repeated(object, 7), b);
    return out;
}
Object Capture(const SampleInterval& sample, const std::optional<Clock::time_point>& read_finished, u64 seq,
               const Binding& b) {
    auto out = Interval(sample, seq, b);
    if (read_finished) {
        Require(*read_finished >= sample.started && *read_finished <= sample.finished,
                "Read-finished anchor is outside current capture", ErrorCode::InvalidClock);
        u64 qpc = 0;
        Error error;
        Require(ToQpc(b.clock, *read_finished, true, qpc, error), error.message, ErrorCode::InvalidClock);
        Require(qpc && qpc >= UInt(out, 2) && qpc <= UInt(out, 3), "Invalid read-finished anchor",
                ErrorCode::InvalidClock);
        Put(out, 5, qpc);
    }
    return out;
}
Object CurrentCommon(bool accepted, const SampleInterval& capture,
                     const std::optional<Clock::time_point>& read_finished, u64 seq, const SourceScope& source,
                     List proofs, const BatchObservation& batch, const Binding& b) {
    Object out;
    Put(out, 1, accepted);
    Put(out, 2, Capture(capture, read_finished, seq, b));
    Put(out, 3, std::move(proofs));
    if (batch.category != DataCategory::Local)
        Put(out, 5, Source(source, batch.directory_generation, batch.membership_generation, b));
    Put(out, 6, u64(batch.category));
    Put(out, 7, batch.directory_generation);
    Put(out, 8, batch.classification_generation);
    if (batch.directory_sample.started != Clock::time_point{})
        Put(out, 9, Interval(batch.directory_sample, batch.directory_generation, b));
    if (batch.classification_sample.started != Clock::time_point{})
        Put(out, 10, Interval(batch.classification_sample, batch.classification_generation, b));
    Put(out, 11,
        Object{{1, Value(batch.planned)},
               {2, Value(batch.succeeded)},
               {3, Value(batch.read_failed)},
               {4, Value(batch.dependency_blocked)},
               {5, Value(batch.not_executed)}});
    Put(out, 12, batch.membership_generation);
    if (batch.membership_sample.started != Clock::time_point{})
        Put(out, 13, Interval(batch.membership_sample, batch.membership_generation, b));
    return out;
}
} // namespace
Object PropertyFields(PropertyGroup group, Object out, bool to_wire) {
    if (group == PropertyGroup::CharacterMovement && out.contains(1))
        out.at(1) = Vector(out.at(1), to_wire);
    if (group == PropertyGroup::Spatial)
        for (auto field : {1u, 2u})
            if (out.contains(field))
                out.at(field) = Vector(out.at(field), to_wire);
    if (group == PropertyGroup::ContainerContents && out.contains(1)) {
        auto& collection = std::get<Object>(out.at(1).data);
        if (collection.contains(1))
            for (auto& entry : std::get<List>(collection.at(1).data)) {
                auto& member = std::get<Object>(entry.data);
                if (!member.contains(4))
                    continue;
                auto& spatial = std::get<Object>(member.at(4).data);
                for (unsigned field : {1u, 2u})
                    if (spatial.contains(field))
                        spatial.at(field) = Vector(spatial.at(field), to_wire);
            }
    }
    return out;
}
Object ContextFields(ContextGroup group, const Object& input, bool to_wire) {
    auto out = input;
    if (group == ContextGroup::LocalControllerRotation || group == ContextGroup::LocalNativeSway ||
        group == ContextGroup::LocalAimBasis) {
        const auto field = group == ContextGroup::LocalControllerRotation ? 2u
                           : group == ContextGroup::LocalNativeSway       ? 3u
                                                                          : 6u;
        if (out.contains(field))
            out.at(field) = Vector(out.at(field), to_wire);
        return out;
    }
    if (group == ContextGroup::ViewPose) {
        for (const auto field : {1u, 2u, 4u})
            if (out.contains(field))
                out.at(field) = Vector(out.at(field), to_wire);
        return out;
    }
    const unsigned field = group == ContextGroup::Coordinates ? 3 : group == ContextGroup::LocalControl ? 8 : 0;
    if (field && out.contains(field))
        out.at(field) = Vector(out.at(field), to_wire);
    return out;
}
Object Interval(const SampleInterval& sample, u64 seq, const Binding& b) {
    Require(seq && sample.started != Clock::time_point{} && sample.finished >= sample.started,
            "Invalid sample interval", ErrorCode::InvalidClock);
    Error error;
    Require(ValidateClock(b.clock, error), error.message, ErrorCode::InvalidClock);
    u64 start = 0, end = 0;
    Require(ToQpc(b.clock, sample.started, false, start, error), error.message, ErrorCode::InvalidClock);
    Require(ToQpc(b.clock, sample.finished, true, end, error), error.message, ErrorCode::InvalidClock);
    return Object{{1, Value(seq)}, {2, Value(start)}, {3, Value(end)}, {4, Value(b.clock.mapping_revision)}};
}
SampleInterval ReadInterval(const Object& object, const Binding& b, u64& seq) {
    Error error;
    Require(ValidateClock(b.clock, error), error.message, ErrorCode::InvalidClock);
    seq = UInt(object, 1);
    const auto start = UInt(object, 2), end = UInt(object, 3);
    Require(seq && start && end >= start && UInt(object, 4) == b.clock.mapping_revision,
            "Invalid interval clock/sequence", ErrorCode::InvalidClock);
    SampleInterval out;
    Require(FromQpc(b.clock, start, out.started, error), error.message, ErrorCode::InvalidClock);
    Require(FromQpc(b.clock, end, out.finished, error), error.message, ErrorCode::InvalidClock);
    return out;
}
List NestedList(const std::map<std::vector<unsigned>, GroupState::NestedSample>& samples, const Binding& b) {
    List out;
    for (const auto& [path, s] : samples) {
        Require(!path.empty() && unsigned(s.latest_status) >= 1 && unsigned(s.latest_status) <= 5,
                "Invalid nested observation/path");
        if (s.value_status)
            Require(*s.value_status == OwnedReadStatus::Available || *s.value_status == OwnedReadStatus::Partial,
                    "Invalid nested value status");
        Object evidence;
        if (s.value_status) {
            Put(evidence, 1, Interval(s.source, s.capture_sequence, b));
            Put(evidence, 3, u64(*s.value_status));
        }
        if (s.attempt.started != Clock::time_point{})
            Put(evidence, 2, Interval(s.attempt, s.capture_sequence, b));
        Put(evidence, 4, u64(s.latest_status));
        List route;
        for (auto field : path)
            route.emplace_back(u64(field));
        out.emplace_back(Object{{1, Value(std::move(route))}, {2, Value(std::move(evidence))}});
    }
    return out;
}
std::map<std::vector<unsigned>, GroupState::NestedSample> ReadNested(const List& list, const Binding& b) {
    std::map<std::vector<unsigned>, GroupState::NestedSample> out;
    for (const auto& value : list) {
        const auto& object = std::get<Object>(value.data);
        std::vector<unsigned> path;
        for (const auto& field : Repeated(object, 1)) {
            const auto number = std::get<u64>(field.data);
            Require(number && number <= UINT32_MAX, "Invalid nested path");
            path.push_back(unsigned(number));
        }
        Require(!path.empty(), "Empty nested path");
        const auto& obs = Message(object, 2);
        GroupState::NestedSample s;
        s.latest_status = OwnedReadStatus(UInt(obs, 4));
        Require(unsigned(s.latest_status) >= 1 && unsigned(s.latest_status) <= 5, "Unknown nested observation");
        u64 seq = 0;
        if (obs.contains(2))
            s.attempt = ReadInterval(Message(obs, 2), b, s.capture_sequence);
        if (obs.contains(3)) {
            const auto status = UInt(obs, 3);
            Require(status == 1 || status == 2, "Invalid nested value status");
            s.value_status = OwnedReadStatus(status);
            Require(obs.contains(1), "Nested value without source sample");
            s.source = ReadInterval(Message(obs, 1), b, seq);
            Require(seq == s.capture_sequence, "Nested value/attempt capture mismatch");
        } else
            Require(!obs.contains(1), "Nested source sample without value");
        Require(out.emplace(std::move(path), std::move(s)).second, "Duplicate nested sample path");
    }
    return out;
}
Object Instance(const InstanceIdentity& i) {
    Require(i.actor_address && i.internal_index >= 0, "Invalid native instance identity");
    return {{1, Value(i.actor_address)}, {2, Value(std::int64_t(i.internal_index))}, {3, Value(u64(i.serial_number))}};
}
InstanceIdentity ReadInstance(const Object& object) {
    Require(object.contains(1) && object.contains(2) && object.contains(3), "Incomplete native instance identity");
    const auto index = Signed(object, 2);
    const auto serial = UInt(object, 3);
    Require(index >= 0 && index <= INT32_MAX && serial <= UINT32_MAX && UInt(object, 1),
            "Invalid native instance identity");
    return {UInt(object, 1), i32(index), u32(serial)};
}
Object Entity(const EntityState& e, const Binding& b) {
    const auto contract = b.contract.find(e.key.kind);
    Require(e.id && contract != b.contract.end() && e.groups.size() == contract->second.enabled.size(),
            "Invalid/incomplete entity contract");
    for (auto group : contract->second.enabled)
        Require(e.groups.contains(group), "Entity omitted negotiated terminal group");
    Object out;
    Put(out, 1, e.id);
    Put(out, 2, u64(e.key.kind));
    Put(out, 3, Instance(e.key.instance));
    List values;
    for (const auto& [group, sample] : e.groups)
        values.emplace_back(Property(group, sample, b));
    Put(out, 4, std::move(values));
    return out;
}
EntityState ReadEntity(const Object& object, const Binding& b) {
    EntityState out;
    out.id = UInt(object, 1);
    out.key = {EntityKind(UInt(object, 2)), ReadInstance(Message(object, 3))};
    Require(out.id && unsigned(out.key.kind) >= 1 && unsigned(out.key.kind) <= unsigned(EntityKind::Container),
            "Invalid entity record");
    for (const auto& value : Repeated(object, 4)) {
        PropertyGroup group;
        auto sample = ReadProperty(std::get<Object>(value.data), group, b);
        Require(GroupAllowed(out.key.kind, group) && out.groups.emplace(group, std::move(sample)).second,
                "Duplicate/invalid entity property");
    }
    return out;
}
namespace {
bool LocalGroup(ContextGroup group) {
    return IsLocalContextGroup(group);
}
Object ContextGroupValue(ContextGroup kind, const GroupState& group, const Binding& binding, bool typed) {
    Require(FactStatusToWire(group.status) && !(FieldMask(group.fields) & ~AllowedContextFields(kind)) &&
                group.field_sources.size() == group.fields.size(),
            "Invalid context state domain");
    Require(group.status == FactStatus::Available || group.status == FactStatus::Partial || group.fields.empty(),
            "Unavailable context has current values");
    Object out;
    if (typed) {
        Put(out, 1, u64(FactStatusToWire(group.status)));
        Put(out, 2, FieldMask(group.fields));
        if (!group.fields.empty())
            Put(out, 3, ContextFields(kind, group.fields, true));
    } else {
        Put(out, 1, u64(kind));
        Put(out, 2, u64(FactStatusToWire(group.status)));
        Put(out, 3, FieldMask(group.fields));
        if (!group.fields.empty())
            Put(out, 4, Object{{unsigned(kind), Value(ContextFields(kind, group.fields, true))}});
    }
    List sources;
    for (const auto& [field, source] : group.field_sources) {
        Require(group.fields.contains(field) && source.origin >= 1 && source.origin <= 3,
                "Invalid context field source");
        sources.emplace_back(Object{{1, Value(u64(field))},
                                    {2, Value(Interval(source.sample, source.capture_sequence, binding))},
                                    {3, Value(source.reused)},
                                    {4, Value(u64(source.origin))},
                                    {5, Value(u64(source.source_category))}});
    }
    Put(out, typed ? 4u : 5u, std::move(sources));
    Put(out, typed ? 5u : 6u, Interval(group.verified_sample, group.verified_capture_sequence, binding));
    if (kind == ContextGroup::LocalWeapon)
        Put(out, 6, NestedList(group.nested_samples, binding));
    else
        Require(group.nested_samples.empty(), "Unexpected nested context samples");
    return out;
}
GroupState ReadContextGroup(ContextGroup kind, const Object& message, const Binding& binding, bool typed) {
    const auto status = FactStatusFromWire(unsigned(UInt(message, typed ? 1u : 2u)));
    Require(bool(status), "Unknown context terminal status");
    GroupState group;
    group.status = *status;
    if (message.contains(typed ? 3u : 4u)) {
        if (typed)
            group.fields = ContextFields(kind, Message(message, 3), false);
        else {
            const auto& wrapper = Message(message, 4);
            Require(wrapper.size() == 1 && wrapper.contains(unsigned(kind)), "Context oneof mismatch");
            group.fields = ContextFields(kind, Message(wrapper, unsigned(kind)), false);
        }
    }
    const auto mask = UInt(message, typed ? 2u : 3u);
    Require(mask == FieldMask(group.fields) && !(mask & ~AllowedContextFields(kind)), "Context presence mask mismatch");
    for (const auto& value : Repeated(message, typed ? 4u : 5u)) {
        const auto& source = std::get<Object>(value.data);
        const auto field = UInt(source, 1);
        Require(field >= 1 && field <= 64 && group.fields.contains(unsigned(field)),
                "Context source without current field");
        GroupState::FieldSource entry;
        entry.sample = ReadInterval(Message(source, 2), binding, entry.capture_sequence);
        entry.reused = Boolean(source, 3);
        entry.origin = unsigned(UInt(source, 4));
        entry.source_category = DataCategory(UInt(source, 5));
        Require(entry.origin >= 1 && entry.origin <= 3 && group.field_sources.emplace(unsigned(field), entry).second,
                "Duplicate or invalid context field source");
        if (typed)
            Require(!entry.reused && entry.source_category == DataCategory::Local,
                    "Local value has a reused or cross-category field source");
    }
    group.verified_sample = ReadInterval(Message(message, typed ? 5u : 6u), binding, group.verified_capture_sequence);
    if (kind == ContextGroup::LocalWeapon)
        group.nested_samples = ReadNested(Repeated(message, 6), binding);
    return group;
}
} // namespace
Object Context(const FrameContext& context, const Binding& binding) {
    const bool local = context.contains(ContextGroup::LocalControl);
    Require(context.size() == ExpectedContextGroups(context, local), "Missing context terminal group");
    List common;
    Object aggregate, camera;
    for (const auto& [kind, group] : context) {
        Require(AllowedContextFields(kind) != 0, "Unknown context group");
        const bool typed = LocalGroup(kind);
        Require(local || !typed, "Actor context contains local-only values");
        auto value = ContextGroupValue(kind, group, binding, typed);
        if (!typed)
            common.emplace_back(std::move(value));
        else {
            const auto* slot = LocalContextBindingFor(kind);
            Require(slot != nullptr, "Unknown typed Local context group");
            auto& target = slot->in_camera ? camera : aggregate;
            Require(target.emplace(slot->field, Value(std::move(value))).second, "Duplicate typed Local context field");
        }
    }
    std::size_t root_groups = 0, camera_groups = 0;
    if (local)
        for (const auto& slot : LocalContextBindings)
            if (slot.required || context.contains(slot.group))
                ++(slot.in_camera ? camera_groups : root_groups);
    Require(common.size() == 4 && (!local || aggregate.size() == root_groups && camera.size() == camera_groups),
            "Incomplete typed Local context");
    Object out{{1, Value(std::move(common))}};
    if (local) {
        aggregate.emplace(2, Value(std::move(camera)));
        Put(out, 2, std::move(aggregate));
    }
    return out;
}
FrameContext ReadContext(const Object& object, const Binding& binding) {
    FrameContext out;
    for (const auto& value : Repeated(object, 1)) {
        const auto& message = std::get<Object>(value.data);
        const auto kind = ContextGroup(UInt(message, 1));
        Require(unsigned(kind) >= 1 && unsigned(kind) <= 5 && !LocalGroup(kind),
                "Unknown or local-only common context group");
        Require(out.emplace(kind, ReadContextGroup(kind, message, binding, false)).second,
                "Duplicate common context group");
    }
    Require(out.size() == 4, "Missing common context terminal group");
    if (object.contains(2)) {
        const auto& local = Message(object, 2);
        Require(local.contains(2), "Missing typed Local camera state");
        const auto& camera = Message(local, 2);
        std::size_t root_groups = 1, camera_groups = 0;
        for (const auto& slot : LocalContextBindings) {
            const auto& source = slot.in_camera ? camera : local;
            const bool present = source.contains(slot.field);
            Require(!slot.required || present, "Missing required typed Local terminal group");
            if (!present)
                continue;
            ++(slot.in_camera ? camera_groups : root_groups);
            Require(out.emplace(slot.group, ReadContextGroup(slot.group, Message(source, slot.field), binding, true))
                        .second,
                    "Duplicate typed Local terminal group");
        }
        Require(local.size() == root_groups && camera.size() == camera_groups, "Unknown typed Local context field");
    }
    return out;
}
Object Current(const State& state, const Binding& b) {
    List proofs;
    for (const auto& [id, e] : state.entities) {
        EntityProof proof;
        proof.key = e.key;
        for (const auto& [g, s] : e.groups)
            proof.groups[g] = {
                s.status,         FieldMask(s.fields), s.verified_sample,        s.verified_capture_sequence,
                s.nested_samples, s.source_sample,     s.source_capture_sequence};
        proofs.emplace_back(EntityProofValue(id, proof, b));
    }
    return CurrentCommon(state.accepted, state.capture, state.read_finished, state.capture_sequence, state.source,
                         std::move(proofs), state.batch, b);
}
Object Current(const Update& delta, const Binding& b) {
    List proofs;
    for (const auto& [id, p] : delta.proofs) {
        for (const auto& [group, sample] : p.groups)
            Require(sample.verified_capture_sequence == delta.capture_sequence &&
                        sample.verified_sample.started >= delta.capture.started &&
                        sample.verified_sample.finished <= delta.capture.finished,
                    "Proof is outside the current capture");
        proofs.emplace_back(EntityProofValue(id, p, b));
    }
    return CurrentCommon(delta.accepted, delta.capture, delta.read_finished, delta.capture_sequence, delta.source,
                         std::move(proofs), delta.batch, b);
}
void ReadCurrent(const Object& object, Update& out, const Binding& b) {
    out.accepted = Boolean(object, 1);
    const auto& capture = Message(object, 2);
    out.capture = ReadInterval(capture, b, out.capture_sequence);
    out.read_finished.reset();
    if (capture.contains(5)) {
        const auto qpc = UInt(capture, 5);
        Require(qpc && qpc >= UInt(capture, 2) && qpc <= UInt(capture, 3), "Invalid read-finished anchor",
                ErrorCode::InvalidClock);
        Clock::time_point point;
        Error error;
        Require(FromQpc(b.clock, qpc, point, error), error.message, ErrorCode::InvalidClock);
        out.read_finished = point;
    }
    out.batch.category = DataCategory(UInt(object, 6));
    out.batch.directory_generation = UInt(object, 7);
    out.batch.classification_generation = UInt(object, 8);
    u64 directory_sequence = 0, classification_sequence = 0;
    if (object.contains(9))
        out.batch.directory_sample = ReadInterval(Message(object, 9), b, directory_sequence);
    if (object.contains(10))
        out.batch.classification_sample = ReadInterval(Message(object, 10), b, classification_sequence);
    Require(directory_sequence == out.batch.directory_generation &&
                classification_sequence == out.batch.classification_generation,
            "Category source generation differs from observation");
    const auto& tasks = Message(object, 11);
    out.batch.planned = UInt(tasks, 1);
    out.batch.succeeded = UInt(tasks, 2);
    out.batch.read_failed = UInt(tasks, 3);
    out.batch.dependency_blocked = UInt(tasks, 4);
    out.batch.not_executed = UInt(tasks, 5);
    out.batch.membership_generation = UInt(object, 12);
    u64 membership_sequence = 0;
    if (object.contains(13))
        out.batch.membership_sample = ReadInterval(Message(object, 13), b, membership_sequence);
    Require(membership_sequence == out.batch.membership_generation, "Membership observation generation differs");
    Require(object.contains(5) == (out.batch.category != DataCategory::Local),
            "Category source proof presence mismatch");
    if (object.contains(5))
        out.source = ReadSource(Message(object, 5), out.batch.directory_generation, out.batch.membership_generation, b);
    for (const auto& value : Repeated(object, 3)) {
        u64 id = 0;
        auto proof = ReadEntityProof(std::get<Object>(value.data), id, b);
        for (const auto& [group, s] : proof.groups)
            Require(s.verified_capture_sequence == out.capture_sequence, "Proof from another capture");
        Require(out.proofs.emplace(id, std::move(proof)).second, "Duplicate entity verification");
    }
}
void ReadCurrent(const Object& object, State& out, const Binding& b) {
    Update delta;
    ReadCurrent(object, delta, b);
    out.accepted = delta.accepted;
    out.capture = delta.capture;
    out.capture_sequence = delta.capture_sequence;
    out.read_finished = delta.read_finished;
    out.source = delta.source;
    out.batch = delta.batch;
}
Object Baseline(const BaselineOffer& offer, const Binding& b) {
    Require(b.profile == SubscriptionProfile::Debug, "Native evidence is available only in Debug profile");
    Require(offer.state && offer.session == b.session, "Baseline session mismatch");
    std::string error;
    Require(offer.state->accepted, "Rejected frame is not a baseline", ErrorCode::InvalidState);
    Require(ValidateState(*offer.state, error), error, ErrorCode::InvalidState);
    Require(offer.state->contract == b.contract && offer.generation && offer.fence, "Baseline contract/token mismatch");
    Object out;
    Put(out, 1, b.session.session_id);
    Put(out, 2, offer.state->epoch);
    Put(out, 3, offer.generation);
    Put(out, 4, offer.fence);
    Put(out, 5, offer.state->revision);
    Put(out, 6, Current(*offer.state, b));
    List entities;
    for (const auto& [id, e] : offer.state->entities)
        entities.emplace_back(Entity(e, b));
    Put(out, 7, std::move(entities));
    Put(out, 8, Context(offer.state->context, b));
    if (offer.send_started_qpc) {
        ValidateSendStarted(*offer.send_started_qpc, UInt(Message(Message(out, 6), 2), 3), b);
        Put(out, 9, *offer.send_started_qpc);
    }
    return out;
}
BaselineOffer ReadBaseline(const Object& object, const Binding& b) {
    Require(b.profile == SubscriptionProfile::Debug, "Native evidence is available only in Debug profile");
    Require(UInt(object, 1) == b.session.session_id && b.session.session_id, "Baseline session mismatch");
    auto state = std::make_shared<State>();
    state->epoch = UInt(object, 2);
    state->revision = UInt(object, 5);
    state->contract = b.contract;
    Update proof;
    ReadCurrent(Message(object, 6), proof, b);
    state->accepted = proof.accepted;
    state->capture = proof.capture;
    state->capture_sequence = proof.capture_sequence;
    state->read_finished = proof.read_finished;
    state->source = proof.source;
    state->batch = proof.batch;
    state->context = ReadContext(Message(object, 8), b);
    for (const auto& value : Repeated(object, 7)) {
        auto e = ReadEntity(std::get<Object>(value.data), b);
        const auto p = proof.proofs.find(e.id);
        Require(p != proof.proofs.end() && p->second.key == e.key && p->second.groups.size() == e.groups.size(),
                "Baseline value/current proof mismatch");
        for (const auto& [g, s] : e.groups) {
            const auto q = p->second.groups.find(g);
            Require(q != p->second.groups.end() && q->second.status == s.status &&
                        q->second.present_fields == FieldMask(s.fields) &&
                        q->second.verified_capture_sequence == s.verified_capture_sequence &&
                        q->second.verified_sample.started == s.verified_sample.started &&
                        q->second.verified_sample.finished == s.verified_sample.finished &&
                        q->second.source_capture_sequence == s.source_capture_sequence &&
                        q->second.source_sample.started == s.source_sample.started &&
                        q->second.source_sample.finished == s.source_sample.finished,
                    "Baseline property/proof mismatch");
            Require(SameNested(s.nested_samples, q->second.nested_samples), "Baseline nested proof mismatch");
        }
        Require(state->entities.emplace(e.id, std::move(e)).second, "Duplicate baseline entity");
    }
    Require(state->entities.size() == proof.proofs.size(), "Baseline proof without entity");
    std::string error;
    Require(state->accepted, "Rejected frame is not a baseline", ErrorCode::InvalidState);
    Require(ValidateState(*state, error), error, ErrorCode::InvalidState);
    const auto generation = UInt(object, 3), fence = UInt(object, 4);
    Require(generation && fence, "Missing baseline token");
    BaselineOffer out{b.session, generation, fence, std::move(state)};
    if (object.contains(9)) {
        const auto qpc = UInt(object, 9);
        ValidateSendStarted(qpc, UInt(Message(Message(object, 6), 2), 3), b);
        out.send_started_qpc = qpc;
    }
    return out;
}
Object Delta(const Update& delta, const Binding& b) {
    Require(b.profile == SubscriptionProfile::Debug, "Native evidence is available only in Debug profile");
    Require(b.session.session_id && delta.epoch && delta.baseline_generation && delta.fence && delta.base_revision &&
                delta.revision > delta.base_revision,
            "Invalid update token");
    Object out;
    Put(out, 1, b.session.session_id);
    Put(out, 2, delta.epoch);
    Put(out, 3, delta.baseline_generation);
    Put(out, 4, delta.fence);
    Put(out, 5, delta.base_revision);
    Put(out, 6, delta.revision);
    Put(out, 7, Current(delta, b));
    List added, removed, changed;
    for (const auto& [id, e] : delta.additions)
        added.emplace_back(Entity(e, b));
    for (const auto& [id, r] : delta.removals)
        Require(id && unsigned(r) <= 4 && !delta.additions.contains(id), "Invalid/conflicting withdrawal");
    for (const auto& [id, r] : delta.removals)
        removed.emplace_back(Object{{1, Value(id)}, {2, Value(u64(r))}});
    std::set<std::pair<u64, PropertyGroup>> targets;
    for (const auto& patch : delta.patches) {
        const auto mask = FieldMask(patch.values), allowed = AllowedFields(patch.group);
        Require(patch.entity_id && PropertyGroupToWire(patch.group) && unsigned(patch.operation) <= 2 &&
                    !delta.additions.contains(patch.entity_id) && !delta.removals.contains(patch.entity_id) &&
                    targets.emplace(patch.entity_id, patch.group).second && !((mask | patch.clear_fields) & ~allowed) &&
                    !(mask & patch.clear_fields),
                "Invalid/conflicting property patch");
        Require(patch.operation == PatchOperation::Clear     ? !mask && patch.clear_fields == allowed
                : patch.operation == PatchOperation::Replace ? !patch.clear_fields
                                                             : mask || patch.clear_fields,
                "Invalid SET/CLEAR/REPLACE shape");
        Require(patch.values.empty() || patch.source_sample.started != Clock::time_point{},
                "Patch value has no original source");
        Object p;
        Put(p, 1, patch.entity_id);
        Put(p, 2, u64(PropertyGroupToWire(patch.group)));
        Put(p, 3, u64(unsigned(patch.operation) + 1));
        Put(p, 4, FieldMask(patch.values));
        Put(p, 5, patch.clear_fields);
        if (!patch.values.empty()) {
            Object wrapper;
            wrapper.emplace(PropertyGroupToWire(patch.group), Value(PropertyFields(patch.group, patch.values, true)));
            Put(p, 6, std::move(wrapper));
        }
        if (patch.source_sample.started != Clock::time_point{})
            Put(p, 7, Interval(patch.source_sample, patch.source_capture_sequence, b));
        changed.emplace_back(std::move(p));
    }
    Put(out, 8, std::move(added));
    Put(out, 9, std::move(removed));
    Put(out, 10, std::move(changed));
    Put(out, 11, Context(delta.context, b));
    if (delta.send_started_qpc) {
        ValidateSendStarted(*delta.send_started_qpc, UInt(Message(Message(out, 7), 2), 3), b);
        Put(out, 12, *delta.send_started_qpc);
    }
    return out;
}
Update ReadDelta(const Object& object, const Binding& b) {
    Require(b.profile == SubscriptionProfile::Debug, "Native evidence is available only in Debug profile");
    Require(UInt(object, 1) == b.session.session_id && b.session.session_id, "Update session mismatch");
    Update out;
    out.epoch = UInt(object, 2);
    out.baseline_generation = UInt(object, 3);
    out.fence = UInt(object, 4);
    out.base_revision = UInt(object, 5);
    out.revision = UInt(object, 6);
    Require(out.epoch && out.baseline_generation && out.fence && out.base_revision && out.revision > out.base_revision,
            "Invalid update token");
    ReadCurrent(Message(object, 7), out, b);
    if (object.contains(12)) {
        const auto qpc = UInt(object, 12);
        ValidateSendStarted(qpc, UInt(Message(Message(object, 7), 2), 3), b);
        out.send_started_qpc = qpc;
    }
    out.context = ReadContext(Message(object, 11), b);
    for (const auto& value : Repeated(object, 8)) {
        auto e = ReadEntity(std::get<Object>(value.data), b);
        Require(out.additions.emplace(e.id, std::move(e)).second, "Duplicate added entity");
    }
    for (const auto& value : Repeated(object, 9)) {
        const auto& r = std::get<Object>(value.data);
        const auto id = UInt(r, 1), reason = UInt(r, 2);
        Require(id && reason <= 4 && !out.additions.contains(id) &&
                    out.removals.emplace(id, RemovalReason(reason)).second,
                "Duplicate/conflicting entity withdrawal");
    }
    std::set<std::pair<u64, PropertyGroup>> seen;
    for (const auto& value : Repeated(object, 10)) {
        const auto& p = std::get<Object>(value.data);
        PropertyPatch patch;
        patch.entity_id = UInt(p, 1);
        const auto group = PropertyGroupFromWire(unsigned(UInt(p, 2)));
        const auto operation = UInt(p, 3);
        Require(group && operation >= 1 && operation <= 3, "Unknown patch group/operation");
        patch.group = *group;
        patch.operation = PatchOperation(operation - 1);
        patch.clear_fields = UInt(p, 5);
        if (p.contains(6)) {
            const auto& wrapper = Message(p, 6);
            Require(wrapper.size() == 1 && wrapper.contains(PropertyGroupToWire(*group)), "Patch oneof mismatch");
            patch.values = PropertyFields(
                *group, DeclaredPropertyFields(*group, Message(wrapper, PropertyGroupToWire(*group)), UInt(p, 4)),
                false);
        }
        const auto mask = FieldMask(patch.values);
        Require(UInt(p, 4) == mask && !(mask & patch.clear_fields) &&
                    !((mask | patch.clear_fields) & ~AllowedFields(*group)),
                "Patch mask mismatch");
        Require(patch.entity_id && !out.additions.contains(patch.entity_id) &&
                    !out.removals.contains(patch.entity_id) && seen.emplace(patch.entity_id, *group).second,
                "Duplicate/conflicting property patch");
        if (p.contains(7))
            patch.source_sample = ReadInterval(Message(p, 7), b, patch.source_capture_sequence);
        Require(patch.values.empty() || patch.source_sample.started != Clock::time_point{},
                "Patch value without original source interval");
        out.patches.push_back(std::move(patch));
    }
    return out;
}
} // namespace monitor::synchronization::codec::detail
