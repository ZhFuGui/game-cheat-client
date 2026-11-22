#include "synchronization/codec/detail.hpp"
#include "synchronization/codec/consumer_codec.hpp"
#include <type_traits>
#include <cmath>

namespace monitor::synchronization::codec::detail {
namespace {
Object Policy(const TransportPolicy& p) {
    Error error;
    Require(ValidatePolicy(p, error), error.message);
    return {{2, Value(u64(p.max_tcp_record_bytes))},       {4, Value(u64(p.heartbeat_interval_ms))},
            {5, Value(u64(p.heartbeat_timeout_ms))},       {6, Value(u64(p.acknowledgement_timeout_ms))},
            {7, Value(u64(p.record_progress_timeout_ms))}, {9, Value(u64(p.invalidation_delivery_timeout_ms))}};
}
TransportPolicy ReadPolicy(const Object& o) {
    TransportPolicy p{unsigned(UInt(o, 2)), unsigned(UInt(o, 4)), unsigned(UInt(o, 5)),
                      unsigned(UInt(o, 6)), unsigned(UInt(o, 7)), unsigned(UInt(o, 9))};
    Error error;
    Require(ValidatePolicy(p, error), error.message);
    return p;
}
Object Mapping(const ClockMapping& m) {
    Error error;
    Require(ValidateClock(m, error), error.message, ErrorCode::InvalidClock);
    return {{1, Value(m.domain_id)},
            {2, Value(u64(1))},
            {3, Value(m.qpc_frequency)},
            {4, Value(m.mapping_revision)},
            {5, Value(m.reader_monotonic_anchor_ns)},
            {6, Value(m.qpc_anchor)},
            {7, Value(m.max_error_ns)},
            {8, Value(m.valid_from_qpc)},
            {9, Value(m.valid_until_qpc)}};
}
ClockMapping ReadMapping(const Object& o) {
    Require(UInt(o, 2) == 1, "Unsupported clock kind", ErrorCode::InvalidClock);
    ClockMapping m{Text(o, 1), UInt(o, 3), UInt(o, 4), UInt(o, 5), UInt(o, 6), UInt(o, 7), UInt(o, 8), UInt(o, 9)};
    Error error;
    Require(ValidateClock(m, error), error.message, ErrorCode::InvalidClock);
    return m;
}
Object ConsumerClock(const ClockIdentity& c) {
    Error error;
    Require(ValidateClockIdentity(c, error), error.message, ErrorCode::InvalidClock);
    return {{1, Value(c.domain_id)}, {2, Value(u64(c.kind))}, {3, Value(c.frequency)}};
}
ClockIdentity ReadConsumerClock(const Object& o) {
    ClockIdentity c{Text(o, 1), ClockKind(UInt(o, 2)), UInt(o, 3)};
    ConsumerClock(c);
    return c;
}
List Kinds(const KindCapabilities& kinds) {
    std::string error;
    Require(kinds.contract.empty() || ValidateContract(kinds.contract, error), error);
    List out;
    for (const auto& [kind, groups] : kinds.contract) {
        List enabled, implemented;
        for (auto group : groups.enabled)
            enabled.emplace_back(u64(PropertyGroupToWire(group)));
        if (const auto i = kinds.implemented.find(kind); i != kinds.implemented.end())
            for (auto group : i->second) {
                Require(groups.enabled.contains(group), "Implemented group not declared");
                implemented.emplace_back(u64(PropertyGroupToWire(group)));
            }
        out.emplace_back(
            Object{{1, Value(u64(kind))}, {2, Value(std::move(enabled))}, {3, Value(std::move(implemented))}});
    }
    for (const auto& [kind, g] : kinds.implemented)
        Require(kinds.contract.contains(kind), "Implemented kind not declared");
    return out;
}
KindCapabilities ReadKinds(const List& list) {
    KindCapabilities out;
    for (const auto& value : list) {
        const auto& o = std::get<Object>(value.data);
        const auto kind = EntityKind(UInt(o, 1));
        Require(unsigned(kind) >= 1 && unsigned(kind) <= unsigned(EntityKind::Container) &&
                    !out.contract.contains(kind),
                "Duplicate/invalid kind contract");
        KindContract contract;
        for (const auto& g : Repeated(o, 2)) {
            const auto group = PropertyGroupFromWire(unsigned(std::get<u64>(g.data)));
            Require(group && GroupAllowed(kind, *group) && contract.enabled.insert(*group).second,
                    "Duplicate/invalid enabled group");
        }
        out.contract.emplace(kind, contract);
        auto& implemented = out.implemented[kind];
        for (const auto& g : Repeated(o, 3)) {
            const auto group = PropertyGroupFromWire(unsigned(std::get<u64>(g.data)));
            Require(group && contract.enabled.contains(*group) && implemented.insert(*group).second,
                    "Duplicate/invalid implemented group");
        }
    }
    std::string error;
    Require(out.contract.empty() || ValidateContract(out.contract, error), error);
    return out;
}
List BusinessKinds(const BusinessKindContract& kinds, const BusinessKindContract& implemented) {
    List out;
    for (const auto& [kind, groups] : kinds) {
        List enabled, actual;
        for (const auto group : groups) {
            Require(consumer::GroupAllowed(kind, group), "Consumer capability is not a business group");
            enabled.emplace_back(u64(PropertyGroupToWire(group)));
        }
        if (const auto found = implemented.find(kind); found != implemented.end())
            for (const auto group : found->second) {
                Require(groups.contains(group), "Consumer implemented capability was not enabled");
                actual.emplace_back(u64(PropertyGroupToWire(group)));
            }
        out.emplace_back(Value(Object{{1, Value(u64(kind))}, {2, Value(std::move(enabled))},
                                      {3, Value(std::move(actual))}}));
    }
    for (const auto& [kind, groups] : implemented)
        Require(kinds.contains(kind), "Consumer implemented kind was not enabled");
    return out;
}
void ReadBusinessKinds(const List& list, BusinessKindContract& kinds, BusinessKindContract& implemented) {
    for (const auto& entry : list) {
        const auto& value = std::get<Object>(entry.data);
        const auto number = UInt(value, 1);
        Require(number >= 1 && number <= unsigned(EntityKind::Container), "Invalid Consumer capability kind");
        const auto kind = EntityKind(number);
        std::set<PropertyGroup> groups, actual;
        for (const auto& g : Repeated(value, 2)) {
            const auto raw = std::get<u64>(g.data);
            Require(raw <= UINT32_MAX, "Consumer business group overflow");
            const auto group = PropertyGroupFromWire(unsigned(raw));
            Require(group && consumer::GroupAllowed(kind, *group) && groups.insert(*group).second,
                    "Invalid/duplicate Consumer capability");
        }
        for (const auto& g : Repeated(value, 3)) {
            const auto raw = std::get<u64>(g.data);
            Require(raw <= UINT32_MAX, "Consumer implemented group overflow");
            const auto group = PropertyGroupFromWire(unsigned(raw));
            Require(group && groups.contains(*group) && actual.insert(*group).second,
                    "Invalid/duplicate Consumer implemented capability");
        }
        Require(kinds.emplace(kind, std::move(groups)).second, "Duplicate Consumer capability kind");
        implemented.emplace(kind, std::move(actual));
    }
}
void IdentityText(const std::string& s) {
    Require(!s.empty() && s.size() <= 128, "Invalid bounded identity string");
}
void Session(const SessionKey& session, const Binding& b) {
    Require(session == b.session && session.session_id, "Record belongs to another session");
}
void Reason(ResyncReason r) {
    Require(unsigned(r) >= 1 && unsigned(r) <= 7, "Unknown resync reason");
}
void Status(ResultStatus s) {
    Require(unsigned(s) >= 1 && unsigned(s) <= 7, "Unknown control result");
}
double Real(const Object& o, unsigned field) {
    const auto found = o.find(field);
    return found == o.end() ? 0.0 : std::get<double>(found->second.data);
}
Object Pacing(const PacingValue& p) {
    Require(ValidCategoryHz(p.target_hz) && p.target_interval_ns == PeriodForHz(p.target_hz),
            "Invalid adopted category pacing");
    return {{1, Value(p.configuration_revision)}, {2, Value(p.target_hz)}, {3, Value(p.target_interval_ns)}};
}
PacingValue ReadPacing(const Object& o) {
    Require(o.contains(1) && o.contains(2) && o.contains(3), "Missing adopted pacing field");
    PacingValue p{UInt(o, 1), UInt(o, 3), Real(o, 2)};
    Pacing(p);
    return p;
}
Object Rate(const CaptureRateRequest& r) {
    Require(r.request_id, "Missing control request ID");
    Require(r.operation == ControlOperation::Get || r.operation == ControlOperation::Set,
            "Unknown capture control operation");
    // 0、SET缺值和GET附值均有合法protobuf表示；交由业务层回复InvalidArgument，不能断开连接。
    Object out{{1, Value(r.request_id)}, {2, Value(u64(r.operation))}};
    if (r.target_hz)
        out.emplace(3, Value(*r.target_hz));
    if (r.expected_pacing_revision)
        Put(out, 4, *r.expected_pacing_revision);
    return out;
}
CaptureRateRequest ReadRate(const Object& o) {
    CaptureRateRequest r;
    r.request_id = UInt(o, 1);
    r.operation = ControlOperation(UInt(o, 2));
    if (o.contains(3))
        r.target_hz = Real(o, 3);
    if (o.contains(4))
        r.expected_pacing_revision = UInt(o, 4);
    Rate(r);
    return r;
}
void Tokens(u64 epoch, u64 fence, u64 revision) {
    Require(epoch && fence && revision, "Missing reliable state token");
}
void Envelope(const ReliableRecord& r) {
    Require(r.protocol_version == ProtocolVersion && r.record_sequence && unsigned(r.direction) >= 1 && unsigned(r.direction) <= 2,
            "Invalid reliable version/sequence/direction");
}
} // namespace
Object Record(const ReliableRecord& r, const Binding& b) {
    Envelope(r);
    Object out{{1, Value(u64(r.protocol_version))},
               {2, Value(r.record_sequence)},
               {3, Value(u64(r.direction))},
               {4, Value(r.session_id)}};
    std::visit(
        [&](const auto& p) {
            using T = std::decay_t<decltype(p)>;
            if constexpr (std::is_same_v<T, Hello>) {
                Require(r.direction == Direction::Subscriber && r.session_id == 0 && p.protocol_version == ProtocolVersion &&
                            p.nonce && unsigned(p.category) >= 1 && unsigned(p.category) <= 6,
                        "Invalid Hello envelope");
                IdentityText(p.subscriber_id);
                IdentityText(p.subscription_id);
                Require(p.clock_contract_version == ClockContractVersion, "Unsupported clock contract version");
                Require(ValidSubscriptionProfile(p.profile) && ValidSubscriptionFeatures(p.features), "Invalid profile/features");
                Require(p.profile == SubscriptionProfile::Debug || (p.kinds.contract.empty() && p.kinds.implemented.empty()),
                        "Consumer cannot declare Native property contract");
                Object value{{1, Value(u64(p.protocol_version))},
                             {2, Value(p.subscriber_id)},
                             {4, Value(Policy(p.transport))},
                             {6, Value(Kinds(p.kinds))},
                             {7, Value(p.nonce)},
                             {8, Value(p.subscription_id)},
                             {9, Value(u64(p.category))},
                             {10, Value(p.target_hz)},
                             {12, Value(u64(p.clock_contract_version))},
                             {13, Value(u64(p.profile))}, {14, Value(p.features)}};
                if (p.consumer_clock)
                    Put(value, 11, ConsumerClock(*p.consumer_clock));
                Put(out, 5, std::move(value));
            } else if constexpr (std::is_same_v<T, HelloAccepted>) {
                Require(r.direction == Direction::Reader && p.protocol_version == ProtocolVersion && p.session.session_id &&
                            p.epoch && p.nonce && r.session_id == p.session.session_id,
                        "Invalid HelloAccepted envelope");
                IdentityText(p.session.stream_id);
                IdentityText(p.session.subscriber_id);
                Require(p.clock_contract_version == ClockContractVersion, "Unsupported clock contract version");
                Require(ValidSubscriptionProfile(p.profile) && p.features == 0, "Unsupported accepted profile/features");
                if (p.profile == SubscriptionProfile::Consumer)
                    Require(p.kinds.contract.empty() && p.kinds.implemented.empty() &&
                                p.business_kinds == CanonicalBusinessContract(p.category),
                            "Consumer negotiated an invalid Native/business contract");
                else
                    Require(p.business_kinds.empty() && p.business_implemented.empty(), "Debug advertised Consumer capabilities");
                Object value{{1, Value(u64(ProtocolVersion))},
                             {2, Value(p.session.stream_id)},
                             {3, Value(p.session.subscriber_id)},
                             {4, Value(p.session.session_id)},
                             {5, Value(p.epoch)},
                             {6, Value(Policy(p.transport))},
                             {7, Value(Mapping(p.clock))},
                             {8, Value(Kinds(p.kinds))},
                             {9, Value(p.nonce)},
                             {10, Value(p.subscription_id)},
                             {11, Value(u64(p.category))},
                             {12, Value(Pacing({p.configuration_revision, PeriodForHz(p.target_hz), p.target_hz}))},
                             {13, Value(u64(p.clock_contract_version))},
                             {15, Value(u64(p.profile))}, {16, Value(p.features)},
                             {17, Value(BusinessKinds(p.business_kinds, p.business_implemented))}};
                if (p.consumer_clock)
                    Put(value, 14, ConsumerClock(*p.consumer_clock));
                Put(out, 6, std::move(value));
            } else if constexpr (std::is_same_v<T, consumer::BaselineOffer>) {
                Require(r.direction == Direction::Reader && r.session_id == b.session.session_id,
                        "Invalid Consumer baseline envelope");
                Put(out, 19, ConsumerBaseline(p, b));
            } else if constexpr (std::is_same_v<T, consumer::Update>) {
                Require(r.direction == Direction::Reader && r.session_id == b.session.session_id,
                        "Invalid Consumer update envelope");
                Put(out, 20, ConsumerDelta(p, b));
            } else if constexpr (std::is_same_v<T, BaselineOffer>) {
                Require(r.direction == Direction::Reader, "Wrong baseline direction");
                Session(p.session, b);
                Require(r.session_id == b.session.session_id, "Envelope session mismatch");
                Put(out, 7, Baseline(p, b));
            } else if constexpr (std::is_same_v<T, Ack>) {
                Require(r.direction == Direction::Subscriber, "Wrong ACK direction");
                Session(p.session, b);
                Tokens(p.epoch, p.fence, p.base_revision);
                Require(r.session_id == p.session.session_id && p.generation && p.capture_sequence &&
                            p.applied_revision == p.base_revision,
                        "Invalid baseline ACK");
                Put(out, 8,
                    Object{{1, Value(p.session.session_id)},
                           {2, Value(p.epoch)},
                           {3, Value(p.generation)},
                           {4, Value(p.fence)},
                           {5, Value(p.base_revision)},
                           {6, Value(p.applied_revision)},
                           {7, Value(p.capture_sequence)}});
            } else if constexpr (std::is_same_v<T, ResyncRequest>) {
                Reason(p.reason);
                Require(r.direction == Direction::Subscriber && p.request_id && p.session_id &&
                            r.session_id == p.session_id && p.session_id == b.session.session_id,
                        "Invalid resync request");
                Put(out, 9,
                    Object{{1, Value(p.request_id)},
                           {2, Value(p.session_id)},
                           {3, Value(p.epoch)},
                           {4, Value(p.generation)},
                           {5, Value(p.last_applied_revision)},
                           {6, Value(u64(p.reason))}});
            } else if constexpr (std::is_same_v<T, InvalidationMessage>) {
                const auto& pbar = p.barrier;
                Session(pbar.session, b);
                Tokens(pbar.epoch, pbar.fence, pbar.revision);
                Reason(p.reason);
                Require(r.direction == Direction::Reader && r.session_id == pbar.session.session_id,
                        "Invalid reliable failure envelope");
                Put(out, 10,
                    Object{{1, Value(pbar.session.session_id)},
                           {2, Value(pbar.epoch)},
                           {3, Value(pbar.fence)},
                           {4, Value(pbar.revision)},
                           {5, Value(u64(p.reason))},
                           {6, Value(pbar.capture_sequence)}});
            } else if constexpr (std::is_same_v<T, Update>) {
                Require(r.direction == Direction::Reader && r.session_id == b.session.session_id,
                        "Invalid oversized update direction/session");
                Put(out, 11, Delta(p, b));
            } else if constexpr (std::is_same_v<T, CaptureRateRequest>) {
                Require(r.direction == Direction::Subscriber && r.session_id == b.session.session_id && r.session_id,
                        "Invalid capture control envelope");
                Put(out, 12, Rate(p));
            } else if constexpr (std::is_same_v<T, CaptureRateResponse>) {
                Status(p.status);
                Require(r.direction == Direction::Reader && r.session_id == b.session.session_id && r.session_id &&
                            p.request_id && p.message.size() <= 1024,
                        "Invalid control response");
                Object value{{1, Value(p.request_id)},
                             {2, Value(u64(p.status))},
                             {4, Value(p.category_target_hz)},
                             {5, Value(p.message)}};
                if (p.adopted.target_hz)
                    Put(value, 3, Pacing(p.adopted));
                else
                    Require(p.status != ResultStatus::Ok, "Successful control has no adopted pacing");
                value.emplace(6, Value(p.classification_target_hz));
                value.emplace(7, Value(p.directory_target_hz));
                Put(out, 13, std::move(value));
            } else if constexpr (std::is_same_v<T, Heartbeat>) {
                Require(p.nonce && r.session_id && r.session_id == b.session.session_id, "Invalid heartbeat");
                Put(out, 14,
                    Object{{1, Value(p.nonce)},
                           {2, Value(p.reply)},
                           {3, Value(p.last_applied_revision)},
                           {4, Value(p.pending_generation)}});
            } else if constexpr (std::is_same_v<T, ErrorResponse>) {
                Status(p.status);
                Require(r.direction == Direction::Reader && p.status != ResultStatus::Ok && p.message.size() <= 1024 &&
                            (r.session_id == 0 || r.session_id == b.session.session_id),
                        "Invalid error response");
                Put(out, 15,
                    Object{{1, Value(p.request_id)},
                           {2, Value(u64(p.status))},
                           {3, Value(p.message)},
                           {4, Value(p.close_connection)}});
            } else if constexpr (std::is_same_v<T, InvalidationAck>) {
                Tokens(p.epoch, p.fence, p.revision);
                Require(r.direction == Direction::Subscriber && p.session_id && r.session_id == p.session_id &&
                            p.session_id == b.session.session_id,
                        "Invalid failure ACK");
                Put(out, 16,
                    Object{{1, Value(p.session_id)},
                           {2, Value(p.epoch)},
                           {3, Value(p.fence)},
                           {4, Value(p.revision)},
                           {5, Value(p.capture_sequence_floor)}});
            } else if constexpr (std::is_same_v<T, ClockProbe>) {
                Require(r.direction == Direction::Subscriber && r.session_id && r.session_id == b.session.session_id &&
                            p.request_id && p.consumer_sent_tick,
                        "Invalid clock probe");
                Put(out, 17, Object{{1, Value(p.request_id)}, {2, Value(*p.consumer_sent_tick)}});
            } else if constexpr (std::is_same_v<T, ClockProbeReply>) {
                Error error;
                Require(ValidateClock(b.clock, error), error.message, ErrorCode::InvalidClock);
                Require(r.direction == Direction::Reader && r.session_id && r.session_id == b.session.session_id &&
                            p.request_id && p.epoch && p.clock_mapping_revision == b.clock.mapping_revision &&
                            p.consumer_sent_tick && p.reader_received_qpc && *p.reader_received_qpc &&
                            *p.reader_received_qpc >= b.clock.valid_from_qpc &&
                            *p.reader_received_qpc <= b.clock.valid_until_qpc,
                        "Invalid clock probe reply");
                Object value{{1, Value(p.request_id)},
                             {2, Value(p.epoch)},
                             {3, Value(p.clock_mapping_revision)},
                             {4, Value(*p.consumer_sent_tick)},
                             {5, Value(*p.reader_received_qpc)}};
                if (p.reader_sent_qpc) {
                    Require(*p.reader_sent_qpc >= *p.reader_received_qpc &&
                                *p.reader_sent_qpc <= b.clock.valid_until_qpc,
                            "Clock probe reply has reversed source anchors", ErrorCode::InvalidClock);
                    Put(value, 6, *p.reader_sent_qpc);
                }
                Put(out, 18, std::move(value));
            }
        },
        r.payload);
    return out;
}
ReliableRecord ReadRecord(const Object& o, const Binding& b) {
    ReliableRecord r;
    r.protocol_version = unsigned(UInt(o, 1));
    r.record_sequence = UInt(o, 2);
    r.direction = Direction(UInt(o, 3));
    r.session_id = UInt(o, 4);
    Envelope(r);
    unsigned field = 0;
    for (unsigned f = 5; f <= 20; ++f)
        if (o.contains(f)) {
            Require(!field, "Multiple reliable payloads");
            field = f;
        }
    Require(field, "Missing reliable payload");
    const auto& p = Message(o, field);
    switch (field) {
        case 5: {
            Hello h;
            h.protocol_version = unsigned(UInt(p, 1));
            h.subscriber_id = Text(p, 2);
            h.subscription_id = Text(p, 8);
            h.category = DataCategory(UInt(p, 9));
            h.target_hz = Real(p, 10);
            h.transport = ReadPolicy(Message(p, 4));
            h.clock_contract_version = unsigned(UInt(p, 12));
            if (p.contains(11))
                h.consumer_clock = ReadConsumerClock(Message(p, 11));
            h.kinds = ReadKinds(Repeated(p, 6));
            h.nonce = UInt(p, 7);
            const auto profile = UInt(p, 13);
            Require(profile <= 2, "Unknown subscription profile");
            h.profile = profile ? SubscriptionProfile(profile) : SubscriptionProfile::Consumer;
            h.features = UInt(p, 14);
            r.payload = std::move(h);
            break;
        }
        case 6: {
            HelloAccepted h;
            h.protocol_version = unsigned(UInt(p, 1));
            h.session = {Text(p, 2), Text(p, 3), UInt(p, 4)};
            h.epoch = UInt(p, 5);
            h.transport = ReadPolicy(Message(p, 6));
            h.clock = ReadMapping(Message(p, 7));
            h.clock_contract_version = unsigned(UInt(p, 13));
            h.profile = SubscriptionProfile(UInt(p, 15));
            h.features = UInt(p, 16);
            ReadBusinessKinds(Repeated(p, 17), h.business_kinds, h.business_implemented);
            if (p.contains(14))
                h.consumer_clock = ReadConsumerClock(Message(p, 14));
            h.kinds = ReadKinds(Repeated(p, 8));
            h.nonce = UInt(p, 9);
            h.subscription_id = Text(p, 10);
            h.category = DataCategory(UInt(p, 11));
            const auto adopted = ReadPacing(Message(p, 12));
            h.target_hz = adopted.target_hz;
            h.configuration_revision = adopted.configuration_revision;
            r.payload = std::move(h);
            break;
        }
        case 7:
            r.payload = ReadBaseline(p, b);
            break;
        case 8:
            r.payload = Ack{b.session, UInt(p, 2), UInt(p, 3), UInt(p, 4), UInt(p, 5), UInt(p, 6), UInt(p, 7)};
            Require(UInt(p, 1) == b.session.session_id, "ACK payload session mismatch");
            break;
        case 9:
            r.payload =
                ResyncRequest{UInt(p, 1), UInt(p, 2), UInt(p, 3), UInt(p, 4), UInt(p, 5), ResyncReason(UInt(p, 6))};
            break;
        case 10:
            Require(UInt(p, 1) == b.session.session_id, "Failure payload session mismatch");
            r.payload = InvalidationMessage{{b.session, UInt(p, 2), UInt(p, 3), UInt(p, 4), UInt(p, 6)},
                                            ResyncReason(UInt(p, 5))};
            break;
        case 11:
            r.payload = ReadDelta(p, b);
            break;
        case 12:
            r.payload = ReadRate(p);
            break;
        case 13: {
            CaptureRateResponse c;
            c.request_id = UInt(p, 1);
            c.status = ResultStatus(UInt(p, 2));
            if (p.contains(3))
                c.adopted = ReadPacing(Message(p, 3));
            c.category_target_hz = Real(p, 4);
            c.classification_target_hz = Real(p, 6);
            c.directory_target_hz = Real(p, 7);
            c.message = Text(p, 5);
            r.payload = std::move(c);
            break;
        }
        case 14:
            r.payload = Heartbeat{UInt(p, 1), UInt(p, 3), UInt(p, 4), Boolean(p, 2)};
            break;
        case 15:
            r.payload = ErrorResponse{UInt(p, 1), ResultStatus(UInt(p, 2)), Text(p, 3), Boolean(p, 4)};
            break;
        case 16:
            r.payload = InvalidationAck{UInt(p, 1), UInt(p, 2), UInt(p, 3), UInt(p, 4), UInt(p, 5)};
            break;
        case 17: {
            ClockProbe probe;
            probe.request_id = UInt(p, 1);
            if (p.contains(2))
                probe.consumer_sent_tick = UInt(p, 2);
            r.payload = std::move(probe);
            break;
        }
        case 19:
            r.payload = ReadConsumerBaseline(p, b);
            break;
        case 20:
            r.payload = ReadConsumerDelta(p, b);
            break;
        case 18: {
            Require(p.contains(4) && p.contains(5) && p.contains(6), "Clock probe reply lacks actual anchors");
            r.payload = ClockProbeReply{UInt(p, 1), UInt(p, 2), UInt(p, 3), UInt(p, 4), UInt(p, 5), UInt(p, 6)};
            break;
        }
        default:
            Fail("Unknown reliable payload");
    }
    // 复用发送端的纯DTO约束核验，不依赖本地连接阶段或socket；阶段/序号推进仍由owner控制。
    if (field == 7 || field == 11 || field == 19 || field == 20)
        Require(r.direction == Direction::Reader && r.session_id == b.session.session_id && r.session_id,
                "Invalid state envelope direction/session");
    else
        Record(r, b);
    return r;
}
} // namespace monitor::synchronization::codec::detail
