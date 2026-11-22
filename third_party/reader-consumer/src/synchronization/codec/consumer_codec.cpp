#include "synchronization/codec/consumer_codec.hpp"
#include <cmath>
#include <limits>
#include <tuple>
#include <type_traits>

namespace monitor::synchronization::codec {
BusinessKindContract CanonicalBusinessContract(DataCategory category) {
    BusinessKindContract out;
    if (category == DataCategory::Local) return out;
    for (unsigned k = unsigned(EntityKind::Character); k <= unsigned(EntityKind::Container); ++k) {
        const auto kind = EntityKind(k);
        const bool belongs = category == DataCategory::Living ? kind == EntityKind::Character || kind == EntityKind::Creature
                           : category == DataCategory::Vehicles ? kind == EntityKind::Vehicle
                           : category == DataCategory::GroundLoot && (kind == EntityKind::Pickup || kind == EntityKind::Container);
        if (!belongs) continue;
        for (unsigned g = 0; g < unsigned(PropertyGroup::Count); ++g)
            if (consumer::GroupAllowed(kind, PropertyGroup(g))) out[kind].insert(PropertyGroup(g));
    }
    return out;
}
} // namespace monitor::synchronization::codec

namespace monitor::synchronization::codec::detail {
template<class T, class M> struct ConsumerField { unsigned number; M T::* member; };
template<class T> auto ConsumerMembers();
} // namespace monitor::synchronization::codec::detail
#include "synchronization/codec/consumer_members.hpp"

namespace monitor::synchronization::codec::detail {
namespace {
template<class T> struct Optional : std::false_type {};
template<class T> struct Optional<std::optional<T>> : std::true_type { using Type = T; };
template<class T> struct Sequence : std::false_type {};
template<class T> struct Sequence<std::vector<T>> : std::true_type { using Type = T; };
template<class T> Value BusinessValue(const T&);
template<class T> T ReadBusinessValue(const Value&);
template<class T> Object BusinessObject(const T& input) {
    Object out;
    std::apply([&](const auto&... field) {
        ([&] {
            const auto& value = input.*(field.member);
            using M = std::decay_t<decltype(value)>;
            if constexpr (Optional<M>::value) {
                if (value)
                    out.emplace(field.number, BusinessValue(*value));
            } else
                out.emplace(field.number, BusinessValue(value));
        }(), ...);
    }, ConsumerMembers<T>());
    return out;
}
template<class T> T ReadBusinessObject(const Object& input) {
    T out;
    std::apply([&](const auto&... field) {
        ([&] {
            auto& value = out.*(field.member);
            using M = std::decay_t<decltype(value)>;
            const auto found = input.find(field.number);
            if constexpr (Optional<M>::value) {
                if (found != input.end())
                    value = ReadBusinessValue<typename Optional<M>::Type>(found->second);
            } else if constexpr (Sequence<M>::value) {
                if (found != input.end())
                    value = ReadBusinessValue<M>(found->second);
            } else {
                Require(found != input.end(), "Consumer business field is missing");
                value = ReadBusinessValue<M>(found->second);
            }
        }(), ...);
    }, ConsumerMembers<T>());
    return out;
}
template<class T> Value BusinessValue(const T& value) {
    if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, std::string>)
        return Value(value);
    else if constexpr (std::is_floating_point_v<T>) {
        Require(std::isfinite(value), "Nonfinite Consumer value");
        return Value(double(value));
    } else if constexpr (std::is_enum_v<T> || std::is_unsigned_v<T>)
        return Value(u64(value));
    else if constexpr (std::is_integral_v<T>)
        return Value(std::int64_t(value));
    else if constexpr (Sequence<T>::value) {
        List out;
        out.reserve(value.size());
        for (const auto& entry : value)
            out.emplace_back(BusinessValue(entry));
        return Value(std::move(out));
    } else
        return Value(BusinessObject(value));
}
template<class T> T ReadBusinessValue(const Value& value) {
    if constexpr (std::is_same_v<T, bool>)
        return std::get<bool>(value.data);
    else if constexpr (std::is_same_v<T, std::string>)
        return std::get<std::string>(value.data);
    else if constexpr (std::is_floating_point_v<T>) {
        const auto number = std::get<double>(value.data);
        Require(std::isfinite(number) && std::abs(number) <= (std::numeric_limits<T>::max)(),
                "Consumer floating value is outside its type");
        return T(number);
    } else if constexpr (std::is_enum_v<T>) {
        const auto number = std::get<u64>(value.data);
        Require(number <= (std::numeric_limits<std::underlying_type_t<T>>::max)(), "Consumer enum overflow");
        return T(number);
    } else if constexpr (std::is_unsigned_v<T>) {
        const auto number = std::get<u64>(value.data);
        Require(number <= (std::numeric_limits<T>::max)(), "Consumer integer overflow");
        return T(number);
    } else if constexpr (std::is_integral_v<T>) {
        const auto number = std::get<std::int64_t>(value.data);
        Require(number >= (std::numeric_limits<T>::min)() && number <= (std::numeric_limits<T>::max)(),
                "Consumer signed integer overflow");
        return T(number);
    } else if constexpr (Sequence<T>::value) {
        T out;
        const auto& entries = std::get<List>(value.data);
        out.reserve(entries.size());
        for (const auto& entry : entries)
            out.emplace_back(ReadBusinessValue<typename Sequence<T>::Type>(entry));
        return out;
    } else
        return ReadBusinessObject<T>(std::get<Object>(value.data));
}
template<std::size_t I = 0> consumer::GroupValue ReadVariant(unsigned field, const Object& value) {
    if constexpr (I < std::variant_size_v<consumer::GroupValue>) {
        if (field == I + 1)
            return ReadBusinessObject<std::variant_alternative_t<I, consumer::GroupValue>>(value);
        return ReadVariant<I + 1>(field, value);
    } else {
        Fail("Unknown Consumer business value");
    }
}
using Sources = std::map<std::vector<unsigned>, consumer::SourceTiming>;
List WriteSources(const Sources& sources, const Binding& binding) {
    List out;
    out.reserve(sources.size());
    for (const auto& [path, source] : sources) {
        Require(!path.empty() && path.size() <= 32 && FactStatusToWire(source.status),
                "Invalid Consumer field source path/status");
        List route;
        route.reserve(path.size());
        for (const auto index : path)
            route.emplace_back(Value(u64(index)));
        Object value{{1, Value(std::move(route))}, {2, Value(u64(FactStatusToWire(source.status)))},
                     {4, Value(source.reused)}, {5, Value(u64(source.source_category))}};
        if (source.source.started != Clock::time_point{})
            Put(value, 3, Interval(source.source, source.capture_sequence, binding));
        out.emplace_back(Value(std::move(value)));
    }
    return out;
}
Sources ReadSources(const List& list, const Binding& binding) {
    Sources out;
    for (const auto& entry : list) {
        const auto& value = std::get<Object>(entry.data);
        const auto& route = Repeated(value, 1);
        Require(!route.empty() && route.size() <= 32, "Invalid Consumer field source path length");
        std::vector<unsigned> path;
        path.reserve(route.size());
        for (const auto& number : route) {
            const auto index = std::get<u64>(number.data);
            Require(index <= UINT32_MAX, "Consumer field path overflow");
            path.push_back(unsigned(index));
        }
        consumer::SourceTiming timing;
        const auto status_number = UInt(value, 2);
        Require(status_number <= UINT32_MAX, "Consumer source status overflow");
        const auto status = FactStatusFromWire(unsigned(status_number));
        Require(status.has_value(), "Unknown Consumer source status");
        timing.status = *status;
        timing.reused = Boolean(value, 4);
        const auto category = UInt(value, 5);
        Require(category <= UINT32_MAX && (category == 0 || ImplementedCategory(DataCategory(category))),
                "Invalid Consumer source category");
        timing.source_category = DataCategory(category);
        if (value.contains(3))
            timing.source = ReadInterval(Message(value, 3), binding, timing.capture_sequence);
        Require(out.emplace(std::move(path), timing).second, "Duplicate Consumer source path");
    }
    return out;
}
Object WriteGroup(unsigned group, const consumer::GroupSample& sample, const Binding& binding) {
    Require(group && FactStatusToWire(sample.status), "Invalid Consumer business group");
    Object out{{1, Value(u64(group))}, {2, Value(u64(FactStatusToWire(sample.status)))},
               {3, Value(sample.value.has_value())}, {5, Value(WriteSources(sample.sources, binding))}};
    if (sample.source.started != Clock::time_point{})
        Put(out, 4, Interval(sample.source, sample.source_capture_sequence, binding));
    if (sample.value)
        Put(out, 6, ConsumerValue(*sample.value));
    return out;
}
Object WriteRefresh(unsigned group, const consumer::GroupRefresh& sample, const Binding& binding) {
    consumer::GroupSample metadata;
    metadata.status = sample.status;
    metadata.source_capture_sequence = sample.source_capture_sequence;
    metadata.source = sample.source;
    metadata.sources = sample.sources;
    auto out = WriteGroup(group, metadata, binding);
    out.insert_or_assign(3, Value(sample.has_value));
    return out;
}
consumer::GroupRefresh ReadRefresh(const Object& value, const Binding& binding, bool allow_value) {
    Require(allow_value || !value.contains(6), "Consumer current table must not contain business values");
    const auto status_number = UInt(value, 2);
    Require(status_number <= UINT32_MAX, "Consumer group status overflow");
    const auto status = FactStatusFromWire(unsigned(status_number));
    Require(status.has_value(), "Unknown Consumer group status");
    consumer::GroupRefresh out;
    out.status = *status;
    out.has_value = Boolean(value, 3);
    out.sources = ReadSources(Repeated(value, 5), binding);
    if (value.contains(4))
        out.source = ReadInterval(Message(value, 4), binding, out.source_capture_sequence);
    return out;
}
consumer::GroupSample ReadGroup(const Object& value, const Binding& binding) {
    const auto refresh = ReadRefresh(value, binding, true);
    Require(value.contains(6) == refresh.has_value, "Consumer group value presence mismatch");
    consumer::GroupSample out;
    out.status = refresh.status;
    out.source_capture_sequence = refresh.source_capture_sequence;
    out.source = refresh.source;
    out.sources = refresh.sources;
    if (refresh.has_value)
        out.value = ReadConsumerValue(Message(value, 6));
    return out;
}
PropertyGroup BusinessGroup(const Object& value) {
    const auto number = UInt(value, 1);
    Require(number <= UINT32_MAX, "Consumer business group overflow");
    const auto group = PropertyGroupFromWire(unsigned(number));
    Require(group.has_value() && *group != PropertyGroup::Identity, "Consumer cannot claim Native Identity");
    return *group;
}
ContextGroup LocalGroup(const Object& value) {
    const auto group = UInt(value, 1);
    Require(group <= UINT32_MAX && consumer::LocalGroupAllowed(ContextGroup(group)),
            "Unknown Consumer Local business group");
    return ContextGroup(group);
}
Object WriteEntity(const consumer::Entity& entity, const Binding& binding) {
    List groups;
    groups.reserve(entity.groups.size());
    for (const auto& [group, sample] : entity.groups)
        groups.emplace_back(Value(WriteGroup(PropertyGroupToWire(group), sample, binding)));
    return {{1, Value(entity.id)}, {2, Value(u64(entity.kind))}, {3, Value(std::move(groups))}};
}
consumer::Entity ReadConsumerEntity(const Object& value, const Binding& binding) {
    consumer::Entity out;
    out.id = UInt(value, 1);
    const auto kind = UInt(value, 2);
    Require(kind >= 1 && kind <= unsigned(EntityKind::Container), "Unknown Consumer entity kind");
    out.kind = EntityKind(kind);
    for (const auto& entry : Repeated(value, 3)) {
        const auto& group = std::get<Object>(entry.data);
        Require(out.groups.emplace(BusinessGroup(group), ReadGroup(group, binding)).second,
                "Duplicate Consumer entity group");
    }
    return out;
}
Object ContextValues(const consumer::Context& context) {
    Object out{{4, Value(context.coordinate_convention)},
               {7, Value(Object{{1, Value(context.pacing.target_hz)}, {2, Value(context.pacing.configuration_revision)},
                                {3, Value(context.pacing.plan_revision)}})},
               {9, Value(context.native_world_velocity_supported)}};
    if (context.world_token) Put(out, 1, *context.world_token);
    if (context.level_token) Put(out, 2, *context.level_token);
    if (context.map_name) Put(out, 3, *context.map_name);
    if (context.verified_meters_per_unit) out.emplace(5, Value(*context.verified_meters_per_unit));
    if (context.origin) Put(out, 6, BusinessObject(*context.origin));
    if (context.game_state_token) Put(out, 8, *context.game_state_token);
    return out;
}
consumer::Context ReadContextValues(const Object& value) {
    consumer::Context out;
    if (value.contains(1)) out.world_token = UInt(value, 1);
    if (value.contains(2)) out.level_token = UInt(value, 2);
    if (value.contains(3)) out.map_name = Text(value, 3);
    out.coordinate_convention = Text(value, 4);
    if (value.contains(5)) out.verified_meters_per_unit = std::get<double>(value.at(5).data);
    if (value.contains(6)) out.origin = ReadBusinessObject<consumer::Origin>(Message(value, 6));
    const auto& pacing = Message(value, 7);
    out.pacing = {std::get<double>(pacing.at(1).data), UInt(pacing, 2), UInt(pacing, 3)};
    if (value.contains(8)) out.game_state_token = UInt(value, 8);
    out.native_world_velocity_supported = Boolean(value, 9);
    return out;
}
Object WriteContext(const consumer::Context& context, const Binding& binding) {
    List groups;
    for (const auto& [group, sample] : context.local_groups)
        groups.emplace_back(Value(WriteGroup(unsigned(group), sample, binding)));
    return {{1, Value(ContextValues(context))}, {2, Value(std::move(groups))},
            {3, Value(WriteSources(context.sources, binding))}};
}
consumer::Context ReadConsumerContext(const Object& value, const Binding& binding) {
    auto out = ReadContextValues(Message(value, 1));
    out.sources = ReadSources(Repeated(value, 3), binding);
    for (const auto& entry : Repeated(value, 2)) {
        const auto& group = std::get<Object>(entry.data);
        Require(out.local_groups.emplace(LocalGroup(group), ReadGroup(group, binding)).second,
                "Duplicate Consumer Local group");
    }
    return out;
}
template<class T> Object WriteBatch(const T& state, const Binding& binding) {
    auto capture = Interval(state.capture, state.capture_sequence, binding);
    if (state.read_finished) {
        Require(*state.read_finished >= state.capture.started && *state.read_finished <= state.capture.finished,
                "Consumer read anchor outside capture", ErrorCode::InvalidClock);
        u64 qpc = 0;
        Error error;
        Require(ToQpc(binding.clock, *state.read_finished, true, qpc, error), error.message, ErrorCode::InvalidClock);
        Put(capture, 5, qpc);
    }
    return {{1, Value(std::move(capture))}, {2, Value(u64(state.category))}, {3, Value(state.features)}};
}
template<class T> void ReadBatch(const Object& value, T& state, const Binding& binding) {
    const auto& capture = Message(value, 1);
    state.capture = ReadInterval(capture, binding, state.capture_sequence);
    const auto category = UInt(value, 2);
    Require(category <= UINT32_MAX && ImplementedCategory(DataCategory(category)), "Unsupported Consumer category");
    state.category = DataCategory(category);
    state.features = UInt(value, 3);
    Require(state.features == 0, "Unsupported Consumer feature selection");
    if (capture.contains(5)) {
        Clock::time_point finished;
        Error error;
        Require(FromQpc(binding.clock, UInt(capture, 5), finished, error), error.message, ErrorCode::InvalidClock);
        Require(finished >= state.capture.started && finished <= state.capture.finished,
                "Consumer read anchor outside capture", ErrorCode::InvalidClock);
        state.read_finished = finished;
    }
}
void ConsumerBinding(const Binding& binding) {
    Require(binding.profile == SubscriptionProfile::Consumer && binding.session.session_id &&
                binding.contract.empty() && binding.features == 0 && ImplementedCategory(binding.category) &&
                binding.business_kinds == CanonicalBusinessContract(binding.category),
            "Consumer payload requires the matching Consumer contract");
}
void ConsumerContract(const consumer::State& state, const Binding& binding) {
    Require(state.category == binding.category && state.features == binding.features, "Consumer batch contract mismatch");
    for (const auto& [id, entity] : state.entities) {
        const auto enabled = binding.business_kinds.find(entity.kind);
        Require(enabled != binding.business_kinds.end() && entity.groups.size() == enabled->second.size(),
                "Consumer entity omitted negotiated business group terminal");
        for (const auto group : enabled->second)
            Require(entity.groups.contains(group), "Consumer entity omitted negotiated business group terminal");
    }
}
Object Change(u64 id, unsigned group, const std::optional<consumer::GroupValue>& value) {
    Object out{{1, Value(id)}, {2, Value(u64(group))}, {3, Value(!value.has_value())}};
    if (value) Put(out, 4, ConsumerValue(*value));
    return out;
}
std::optional<consumer::GroupValue> ReadChange(const Object& value) {
    const bool clear = Boolean(value, 3);
    Require(value.contains(4) != clear, "Consumer change must provide either clear or value");
    return clear ? std::nullopt : std::optional<consumer::GroupValue>(ReadConsumerValue(Message(value, 4)));
}
} // namespace
Object ConsumerValue(const consumer::GroupValue& value) {
    Object out;
    std::visit([&](const auto& business) { out.emplace(unsigned(value.index()) + 1, Value(BusinessObject(business))); }, value);
    return out;
}
consumer::GroupValue ReadConsumerValue(const Object& value) {
    Require(value.size() == 1, "Consumer business value must contain exactly one typed value");
    return ReadVariant(value.begin()->first, std::get<Object>(value.begin()->second.data));
}
Object ConsumerBaseline(const consumer::BaselineOffer& offer, const Binding& binding) {
    ConsumerBinding(binding);
    Require(offer.session == binding.session && offer.state && offer.generation && offer.fence,
            "Consumer baseline session/token mismatch");
    std::string error;
    Require(offer.state->accepted && consumer::ValidateState(*offer.state, error), error, ErrorCode::InvalidState);
    ConsumerContract(*offer.state, binding);
    List entities;
    entities.reserve(offer.state->entities.size());
    for (const auto& [id, entity] : offer.state->entities)
        entities.emplace_back(Value(WriteEntity(entity, binding)));
    Object out{{1, Value(offer.session.session_id)}, {2, Value(offer.state->epoch)}, {3, Value(offer.generation)},
               {4, Value(offer.fence)}, {5, Value(offer.state->revision)}, {6, Value(WriteBatch(*offer.state, binding))},
               {7, Value(WriteContext(offer.state->context, binding))}, {8, Value(std::move(entities))}};
    if (offer.send_started_qpc) {
        u64 finished = 0;
        Error clock_error;
        Require(ToQpc(binding.clock, offer.state->capture.finished, true, finished, clock_error), clock_error.message);
        ValidateSendStarted(*offer.send_started_qpc, finished, binding);
        Put(out, 9, *offer.send_started_qpc);
    }
    return out;
}
consumer::BaselineOffer ReadConsumerBaseline(const Object& value, const Binding& binding) {
    ConsumerBinding(binding);
    Require(UInt(value, 1) == binding.session.session_id, "Consumer baseline session mismatch");
    auto state = std::make_shared<consumer::State>();
    state->epoch = UInt(value, 2);
    state->revision = UInt(value, 5);
    state->accepted = true;
    ReadBatch(Message(value, 6), *state, binding);
    state->context = ReadConsumerContext(Message(value, 7), binding);
    for (const auto& entry : Repeated(value, 8)) {
        auto entity = ReadConsumerEntity(std::get<Object>(entry.data), binding);
        Require(state->entities.emplace(entity.id, std::move(entity)).second, "Duplicate Consumer entity ID");
    }
    std::string error;
    Require(consumer::ValidateState(*state, error), error, ErrorCode::InvalidState);
    ConsumerContract(*state, binding);
    consumer::BaselineOffer out{binding.session, UInt(value, 3), UInt(value, 4), std::move(state)};
    Require(out.generation && out.fence, "Consumer baseline token missing");
    if (value.contains(9)) out.send_started_qpc = UInt(value, 9);
    if (out.send_started_qpc) {
        u64 finished = 0;
        Error clock_error;
        Require(ToQpc(binding.clock, out.state->capture.finished, true, finished, clock_error), clock_error.message);
        ValidateSendStarted(*out.send_started_qpc, finished, binding);
    }
    return out;
}
Object ConsumerDelta(const consumer::Update& update, const Binding& binding) {
    ConsumerBinding(binding);
    Require(update.accepted && update.epoch && update.baseline_generation && update.fence &&
                update.base_revision && update.revision > update.base_revision && update.capture_sequence,
            "Invalid Consumer update token");
    Require(update.category == binding.category && update.features == binding.features, "Consumer update contract mismatch");
    Object context{{3, Value(WriteSources(update.context.sources, binding))}};
    if (update.replace_context_values)
        Put(context, 1, ContextValues(update.context));
    List local;
    for (const auto& [group, refresh] : update.local_current)
        local.emplace_back(Value(WriteRefresh(unsigned(group), refresh, binding)));
    Put(context, 2, std::move(local));
    List current, additions, removals, changes, local_changes;
    for (const auto& [id, entity] : update.current) {
        List groups;
        for (const auto& [group, refresh] : entity.groups)
            groups.emplace_back(Value(WriteRefresh(PropertyGroupToWire(group), refresh, binding)));
        current.emplace_back(Value(Object{{1, Value(id)}, {2, Value(u64(entity.kind))}, {3, Value(std::move(groups))}}));
    }
    for (const auto& [id, entity] : update.additions)
        additions.emplace_back(Value(WriteEntity(entity, binding)));
    for (const auto& [id, reason] : update.removals)
        removals.emplace_back(Value(Object{{1, Value(id)}, {2, Value(u64(reason))}}));
    for (const auto& [key, value] : update.changes)
        changes.emplace_back(Value(Change(key.first, PropertyGroupToWire(key.second), value)));
    for (const auto& [group, value] : update.local_changes)
        local_changes.emplace_back(Value(Change(0, unsigned(group), value)));
    Object out{{1, Value(binding.session.session_id)}, {2, Value(update.epoch)}, {3, Value(update.baseline_generation)},
               {4, Value(update.fence)}, {5, Value(update.base_revision)}, {6, Value(update.revision)},
               {7, Value(WriteBatch(update, binding))}, {8, Value(std::move(context))},
               {9, Value(std::move(current))}, {10, Value(std::move(additions))}, {11, Value(std::move(removals))},
               {12, Value(std::move(changes))}, {13, Value(std::move(local_changes))},
               {14, Value(update.replace_context_values)}};
    if (update.send_started_qpc) {
        u64 finished = 0;
        Error error;
        Require(ToQpc(binding.clock, update.capture.finished, true, finished, error), error.message);
        ValidateSendStarted(*update.send_started_qpc, finished, binding);
        Put(out, 15, *update.send_started_qpc);
    }
    return out;
}
consumer::Update ReadConsumerDelta(const Object& value, const Binding& binding) {
    ConsumerBinding(binding);
    Require(UInt(value, 1) == binding.session.session_id, "Consumer update session mismatch");
    consumer::Update out;
    out.epoch = UInt(value, 2);
    out.baseline_generation = UInt(value, 3);
    out.fence = UInt(value, 4);
    out.base_revision = UInt(value, 5);
    out.revision = UInt(value, 6);
    out.accepted = true;
    ReadBatch(Message(value, 7), out, binding);
    Require(out.category == binding.category && out.features == binding.features, "Consumer update contract mismatch");
    out.replace_context_values = Boolean(value, 14);
    const auto& context = Message(value, 8);
    Require(context.contains(1) == out.replace_context_values, "Consumer context replacement presence mismatch");
    if (out.replace_context_values)
        out.context = ReadContextValues(Message(context, 1));
    out.context.sources = ReadSources(Repeated(context, 3), binding);
    for (const auto& entry : Repeated(context, 2)) {
        const auto& group = std::get<Object>(entry.data);
        Require(out.local_current.emplace(LocalGroup(group), ReadRefresh(group, binding, false)).second,
                "Duplicate Consumer Local metadata");
    }
    for (const auto& entry : Repeated(value, 9)) {
        const auto& entity = std::get<Object>(entry.data);
        consumer::EntityRefresh refresh;
        const auto kind = UInt(entity, 2);
        Require(kind >= 1 && kind <= unsigned(EntityKind::Container), "Unknown Consumer current entity kind");
        refresh.kind = EntityKind(kind);
        for (const auto& group_value : Repeated(entity, 3)) {
            const auto& group = std::get<Object>(group_value.data);
            Require(refresh.groups.emplace(BusinessGroup(group), ReadRefresh(group, binding, false)).second,
                    "Duplicate Consumer current group");
        }
        Require(out.current.emplace(UInt(entity, 1), std::move(refresh)).second, "Duplicate Consumer current entity ID");
    }
    for (const auto& entry : Repeated(value, 10)) {
        auto entity = ReadConsumerEntity(std::get<Object>(entry.data), binding);
        Require(out.additions.emplace(entity.id, std::move(entity)).second, "Duplicate Consumer addition");
    }
    for (const auto& entry : Repeated(value, 11)) {
        const auto& removal = std::get<Object>(entry.data);
        const auto reason = UInt(removal, 2);
        Require(reason <= unsigned(RemovalReason::InstanceReplaced) &&
                    out.removals.emplace(UInt(removal, 1), RemovalReason(reason)).second,
                "Invalid/duplicate Consumer withdrawal");
    }
    for (const auto& entry : Repeated(value, 12)) {
        const auto& patch = std::get<Object>(entry.data);
        Object group{{1, Value(UInt(patch, 2))}};
        Require(out.changes.emplace(std::make_pair(UInt(patch, 1), BusinessGroup(group)), ReadChange(patch)).second,
                "Duplicate Consumer entity change");
    }
    for (const auto& entry : Repeated(value, 13)) {
        const auto& patch = std::get<Object>(entry.data);
        Object group{{1, Value(UInt(patch, 2))}};
        Require(UInt(patch, 1) == 0 && out.local_changes.emplace(LocalGroup(group), ReadChange(patch)).second,
                "Invalid/duplicate Consumer Local change");
    }
    if (value.contains(15)) out.send_started_qpc = UInt(value, 15);
    Require(out.epoch && out.baseline_generation && out.fence && out.base_revision &&
                out.revision > out.base_revision && out.capture_sequence,
            "Invalid Consumer update token");
    if (out.send_started_qpc) {
        u64 finished = 0;
        Error error;
        Require(ToQpc(binding.clock, out.capture.finished, true, finished, error), error.message);
        ValidateSendStarted(*out.send_started_qpc, finished, binding);
    }
    return out;
}
} // namespace monitor::synchronization::codec::detail
