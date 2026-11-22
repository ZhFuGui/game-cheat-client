#include "adapters/sources/pubg_reader/session.hpp"
#include "synchronization/codec/detail.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace mousebox::reader
{
namespace d = wire::detail;
namespace
{
void require(bool value, const std::string &message)
{
    if (!value)
        throw std::runtime_error(message);
}
u64 advance(u64 value)
{
    require(value != UINT64_MAX, "Reader record counter exhausted");
    return value + 1;
}
} // namespace
Session::Session(monitor::DataCategory category, std::optional<wire::ClockIdentity> clock, double hz,
                 std::string instance, u64 nonce)
{
    hello_.category = category;
    hello_.consumer_clock = std::move(clock);
    hello_.target_hz = hz;
    hello_.subscriber_id = std::move(instance);
    hello_.subscription_id = monitor::CategoryName(category);
    hello_.nonce = nonce;
    hello_.protocol_version = wire::ProtocolVersion;
    hello_.profile = monitor::SubscriptionProfile::Consumer;
    hello_.features = 0;
}
std::string Session::encode(wire::Payload payload)
{
    wire::ReliableRecord record;
    record.record_sequence = advance(outgoing_);
    record.session_id = binding_.session.session_id;
    record.direction = wire::Direction::Subscriber;
    record.payload = std::move(payload);
    std::string body;
    wire::Error error;
    require(wire::EncodeReliable(record, binding_, maximum(), body, error), error.message);
    outgoing_ = record.record_sequence;
    const auto size = static_cast<unsigned>(body.size());
    std::string packet(4, '\0');
    for (unsigned i = 0; i < 4; ++i)
        packet[i] = static_cast<char>(size >> ((3 - i) * 8));
    packet += body;
    return packet;
}
void Session::accepted(const wire::HelloAccepted &reply)
{
    require(!ready(), "Repeated HelloAccepted");
    require(reply.protocol_version == wire::ProtocolVersion && reply.profile == hello_.profile &&
                reply.features == hello_.features,
            "Reader did not accept Consumer v3 / requested features");
    require(reply.nonce == hello_.nonce && reply.session.subscriber_id == hello_.subscriber_id &&
                reply.subscription_id == hello_.subscription_id && reply.category == hello_.category &&
                reply.consumer_clock == hello_.consumer_clock &&
                reply.clock_contract_version == wire::ClockContractVersion,
            "HelloAccepted does not match request");
    require(reply.kinds.contract.empty() && reply.kinds.implemented.empty(), "Consumer contains a Debug contract");
    require(reply.business_kinds == wire::CanonicalBusinessContract(hello_.category),
            "Unexpected Consumer business contract");
    require(reply.target_hz > 0 && reply.target_hz <= hello_.target_hz, "Unexpected negotiated rate");
    const auto &p = reply.transport;
    const auto &requested = hello_.transport;
    require(p.max_tcp_record_bytes <= requested.max_tcp_record_bytes, "Reader enlarged requested record limit");
    wire::Error error;
    require(wire::ValidateClock(reply.clock, error), error.message);
    require(wire::ValidatePolicy(p, error), error.message);
    binding_.profile = reply.profile;
    binding_.features = reply.features;
    binding_.category = reply.category;
    binding_.business_kinds = reply.business_kinds;
    binding_.clock = reply.clock;
    binding_.session = reply.session;
    binding_.contract.clear();
    policy_ = p;
    sync::ReceiverOptions options;
    options.source_clock_is_local = false;
    receiver_ = std::make_unique<consumer::ReceiverSession>(reply.session, options);
}
void Session::sourceTiming(const sync::Value::Object &envelope, unsigned field, u64 received)
{
    const auto &state = d::Message(envelope, field);
    const auto &current = d::Message(state, field == 19 ? 6 : 7);
    const auto &capture = d::Message(current, 1);
    timing_ = {d::UInt(capture, 1), d::UInt(capture, 2), d::UInt(capture, 3), received, {}, {}};
    if (capture.contains(5))
        timing_.readFinished = d::UInt(capture, 5);
    const auto sentField = field == 19 ? 9U : 15U;
    if (state.contains(sentField))
        timing_.sent = d::UInt(state, sentField);
}
std::vector<wire::Payload> Session::accept(std::string_view body, u64 received)
{
    const auto object = d::DecodeMessage(d::ReliableEnvelopeSchema, body, maximum());
    require(d::UInt(object, 1) == wire::ProtocolVersion && d::UInt(object, 2) == incoming_ &&
                d::UInt(object, 3) == static_cast<unsigned>(wire::Direction::Reader),
            "Reader envelope version/sequence/direction");
    const auto session = d::UInt(object, 4);
    require(!ready() || session == binding_.session.session_id, "Reader envelope session mismatch");
    if (ready() && object.contains(18))
    {
        try
        {
            const auto record = d::ReadRecord(object, binding_);
            acceptProbe(std::get<wire::ClockProbeReply>(record.payload), received);
        }
        catch (const std::exception &e)
        {
            diagnosticError_ = e.what();
            pendingProbe_.reset();
            probe_.reset();
        }
        incoming_ = advance(incoming_);
        return {};
    }
    const auto record = d::ReadRecord(object, binding_);
    std::vector<wire::Payload> replies;
    std::string error;
    if (const auto value = std::get_if<wire::HelloAccepted>(&record.payload))
    {
        require(session == value->session.session_id, "HelloAccepted session mismatch");
        accepted(*value);
    }
    else
    {
        require(ready(), "Reader payload before HelloAccepted");
        if (const auto baseline = std::get_if<consumer::BaselineOffer>(&record.payload))
        {
            const auto before = current();
            const auto ack = receiver_->AcceptBaseline(*baseline, error);
            require(ack.has_value(), error);
            replies.emplace_back(*ack);
            ++baselines_;
            // Same-token retransmission acknowledges the fixed baseline without refreshing current timing.
            if (current() != before)
                sourceTiming(object, 19, received);
        }
        else if (const auto update = std::get_if<consumer::Update>(&record.payload))
        {
            require(receiver_->Apply(binding_.session, *update, error), error);
            sourceTiming(object, 20, received);
            ++updates_;
        }
        else if (const auto invalidation = std::get_if<wire::InvalidationMessage>(&record.payload))
        {
            require(receiver_->Invalidate(invalidation->barrier), "Invalid reliable invalidation");
            const auto &b = invalidation->barrier;
            failureFloor_ = b.epoch == failureEpoch_ ? std::max(failureFloor_, b.capture_sequence) : b.capture_sequence;
            failureEpoch_ = b.epoch;
            timing_ = {};
            ++invalidations_;
            replies.emplace_back(
                wire::InvalidationAck{b.session.session_id, b.epoch, b.fence, b.revision, failureFloor_});
        }
        else if (const auto heartbeat = std::get_if<wire::Heartbeat>(&record.payload))
        {
            if (!heartbeat->reply)
                replies.emplace_back(wire::Heartbeat{heartbeat->nonce, lastRevision(), 0, true});
            else
            {
                require(pendingHeartbeat_ && *pendingHeartbeat_ == heartbeat->nonce, "Heartbeat echo mismatch");
                pendingHeartbeat_.reset();
                ++heartbeatReplies_;
            }
        }
        else if (const auto failure = std::get_if<wire::ErrorResponse>(&record.payload))
        {
            require(!failure->close_connection, failure->message);
            diagnosticError_ = failure->message;
        }
        else
            require(false, "Unexpected Reader payload");
    }
    incoming_ = advance(incoming_);
    return replies;
}
std::optional<wire::Heartbeat> Session::prepareHeartbeat(u64 nonce)
{
    if (!ready() || pendingHeartbeat_)
        return {};
    require(nonce != 0, "Zero heartbeat nonce");
    pendingHeartbeat_ = nonce;
    return wire::Heartbeat{nonce, lastRevision(), 0, false};
}
std::optional<wire::ClockProbe> Session::prepareProbe()
{
    if (!ready() || !hello_.consumer_clock || pendingProbe_)
        return {};
    probeId_ = advance(probeId_);
    pendingProbe_ = wire::ClockProbe{probeId_, u64(0)};
    return pendingProbe_;
}
void Session::probeSent(u64 tick)
{
    require(pendingProbe_.has_value(), "No pending probe");
    pendingProbe_->consumer_sent_tick = tick;
}
void Session::expireProbe()
{
    pendingProbe_.reset();
    probe_.reset();
    diagnosticError_ = "Clock probe timeout";
}
std::optional<control::SourceClockEstimate> Session::sourceClockEstimate() const
{
    if (!ready() || !probe_ || !hello_.consumer_clock)
        return {};
    const auto &p = *probe_;
    const auto frequency = hello_.consumer_clock->frequency;
    monitor::Clock::time_point started, finished;
    wire::Error error;
    if (!wire::FromQpc(binding_.clock, p.t1, started, error) || !wire::FromQpc(binding_.clock, p.t2, finished, error))
        return {};
    const auto first = std::chrono::duration_cast<std::chrono::nanoseconds>(started.time_since_epoch()).count();
    const auto last = std::chrono::duration_cast<std::chrono::nanoseconds>(finished.time_since_epoch()).count();
    if (first <= 0 || last < first || !frequency || !std::isfinite(p.netRttMs) || p.netRttMs < 0)
        return {};
    // The paired midpoints estimate the offset; network asymmetry remains in
    // the uncertainty, rather than presenting half-RTT as measured one-way age.
    const long double localFirst = static_cast<long double>(p.t0) * 1000 / frequency;
    const long double localLast = static_cast<long double>(p.t3) * 1000 / frequency;
    if (!std::isfinite(localLast) || localLast < 0 || localLast >= static_cast<long double>(UINT64_MAX))
        return {};
    control::SourceClockEstimate result;
    result.sourceAnchorNs = first + (last - first) / 2;
    result.localAnchorMs = static_cast<double>(localFirst + (localLast - localFirst) / 2);
    result.uncertaintyMs =
        p.netRttMs / 2 + static_cast<double>(binding_.clock.max_error_ns) / 1e6 + 1; // Local ms quantization.
    result.calibratedAtMs = static_cast<u64>(localLast);
    return result;
}
std::optional<u64> Session::receivedAtMilliseconds() const noexcept
{
    if (!hello_.consumer_clock || !hello_.consumer_clock->frequency || !timing_.received)
        return {};
    const auto frequency = hello_.consumer_clock->frequency, ticks = timing_.received;
    return ticks / frequency * 1000 + static_cast<u64>(static_cast<long double>(ticks % frequency) * 1000 / frequency);
}
void Session::acceptProbe(const wire::ClockProbeReply &reply, u64 received)
{
    require(pendingProbe_ && reply.request_id == pendingProbe_->request_id &&
                reply.consumer_sent_tick == pendingProbe_->consumer_sent_tick,
            "Clock probe echo mismatch");
    require(reply.reader_received_qpc && reply.reader_sent_qpc &&
                reply.clock_mapping_revision == binding_.clock.mapping_revision,
            "Clock probe mapping/anchors");
    const u64 t0 = *reply.consumer_sent_tick, t1 = *reply.reader_received_qpc, t2 = *reply.reader_sent_qpc;
    require(received >= t0 && t2 >= t1, "Clock probe reversed anchors");
    const long double elapsed = static_cast<long double>(received - t0) / hello_.consumer_clock->frequency;
    const long double residence = static_cast<long double>(t2 - t1) / binding_.clock.qpc_frequency;
    const long double net = elapsed - residence;
    require(net >= 0 && std::isfinite(net), "Clock probe negative net RTT");
    probe_ = ProbeSample{reply.request_id, t0, t1, t2, received, static_cast<double>(net * 1000)};
    diagnosticError_.clear();
    pendingProbe_.reset();
}
} // namespace mousebox::reader
