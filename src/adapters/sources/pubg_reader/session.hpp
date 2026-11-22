#pragma once
#include "synchronization/codec/wire.hpp"
#include "games/pubg/model/control_frame.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mousebox::reader
{
namespace sync = monitor::synchronization;
namespace wire = sync::codec;
namespace consumer = sync::consumer;
using monitor::u64;
struct SourceTiming
{
    u64 capture = 0, started = 0, finished = 0, received = 0;
    std::optional<u64> readFinished, sent;
};
struct ProbeSample
{
    u64 id = 0, t0 = 0, t1 = 0, t2 = 0, t3 = 0;
    double netRttMs = 0;
};
class Session
{
  public:
    Session(monitor::DataCategory, std::optional<wire::ClockIdentity>, double hz, std::string instance, u64 nonce);
    wire::Hello hello() const
    {
        return hello_;
    }
    std::string encode(wire::Payload);
    std::vector<wire::Payload> accept(std::string_view body, u64 received);
    std::optional<wire::ClockProbe> prepareProbe();
    void probeSent(u64 tick);
    void expireProbe();
    std::optional<wire::Heartbeat> prepareHeartbeat(u64 nonce);
    bool heartbeatPending() const noexcept
    {
        return pendingHeartbeat_.has_value();
    }
    u64 heartbeatReplies() const noexcept
    {
        return heartbeatReplies_;
    }
    u64 baselineCount() const noexcept
    {
        return baselines_;
    }
    u64 updateCount() const noexcept
    {
        return updates_;
    }
    bool ready() const noexcept
    {
        return receiver_ != nullptr;
    }
    unsigned maximum() const noexcept
    {
        return ready() ? policy_.max_tcp_record_bytes : 16384;
    }
    const wire::TransportPolicy &policy() const noexcept
    {
        return policy_;
    }
    const wire::Binding &binding() const noexcept
    {
        return binding_;
    }
    std::shared_ptr<const consumer::State> current() const
    {
        return receiver_ ? receiver_->Current() : nullptr;
    }
    const SourceTiming &timing() const noexcept
    {
        return timing_;
    }
    std::optional<u64> receivedAtMilliseconds() const noexcept;
    const std::optional<ProbeSample> &probe() const noexcept
    {
        return probe_;
    }
    std::optional<control::SourceClockEstimate> sourceClockEstimate() const;
    const std::string &diagnosticError() const noexcept
    {
        return diagnosticError_;
    }
    u64 lastRevision() const
    {
        auto state = current();
        return state ? state->revision : 0;
    }
    bool probePending() const noexcept
    {
        return pendingProbe_.has_value();
    }
    u64 invalidationCount() const noexcept
    {
        return invalidations_;
    }

  private:
    void accepted(const wire::HelloAccepted &);
    void acceptProbe(const wire::ClockProbeReply &, u64);
    void sourceTiming(const sync::Value::Object &, unsigned stateField, u64 received);
    wire::Hello hello_;
    wire::Binding binding_;
    wire::TransportPolicy policy_;
    std::unique_ptr<consumer::ReceiverSession> receiver_;
    u64 incoming_ = 1, outgoing_ = 0, probeId_ = 0, failureFloor_ = 0, failureEpoch_ = 0, invalidations_ = 0;
    std::optional<wire::ClockProbe> pendingProbe_;
    std::optional<ProbeSample> probe_;
    std::optional<u64> pendingHeartbeat_;
    u64 heartbeatReplies_ = 0, baselines_ = 0, updates_ = 0;
    SourceTiming timing_;
    std::string diagnosticError_;
};
} // namespace mousebox::reader
