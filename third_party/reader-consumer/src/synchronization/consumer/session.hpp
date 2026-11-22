#pragma once
#include "synchronization/consumer/diff.hpp"
#include "synchronization/basic_session.hpp"

namespace monitor::synchronization::consumer {
struct SessionPolicy {
    using State = consumer::State;
    using Update = consumer::Update;
    using Context = consumer::Context;
    static bool Validate(const State& state, std::string& error) { return ValidateState(state, error); }
    static bool Same(const State& a, const State& b) { return SameState(a, b); }
    static bool MergeIdentity(const Context& a, const Context& b, Context& out, std::string& error) {
        return MergeEpochIdentity(a, b, out, error);
    }
    static Update BuildAcknowledgedUpdate(const State& baseline, const State& current, u64 generation, u64 fence) {
        return consumer::BuildAcknowledgedUpdate(baseline, current, generation, fence);
    }
    static bool Apply(const State& baseline, const Update& update, State& out, std::string& error) {
        return ApplyUpdate(baseline, update, out, error);
    }
};
using BaselineOffer = BasicBaselineOffer<State>;
using Emission = BasicEmission<Update>;
using SenderSession = BasicSenderSession<SessionPolicy>;
using ReceiverSession = BasicReceiverSession<SessionPolicy>;
} // namespace monitor::synchronization::consumer
