#pragma once
#include "synchronization/diff.hpp"
#include "synchronization/basic_session.hpp"

namespace monitor::synchronization {
struct DebugSessionPolicy {
    using State = synchronization::State;
    using Update = synchronization::Update;
    using Context = FrameContext;
    static bool Validate(const State&, std::string&);
    static bool Same(const State&, const State&);
    static bool MergeIdentity(const Context&, const Context&, Context&, std::string&);
    static Update BuildAcknowledgedUpdate(const State&, const State&, u64 generation, u64 fence);
    static bool Apply(const State&, const Update&, State&, std::string&);
};
using BaselineOffer = BasicBaselineOffer<State>;
using Emission = BasicEmission<Update>;
using SenderSession = BasicSenderSession<DebugSessionPolicy>;
using ReceiverSession = BasicReceiverSession<DebugSessionPolicy>;
} // namespace monitor::synchronization
