#include "synchronization/session.hpp"
namespace monitor::synchronization {
bool DebugSessionPolicy::Validate(const State& state, std::string& error) { return ValidateState(state, error); }
bool DebugSessionPolicy::Same(const State& a, const State& b) { return SameState(a, b); }
bool DebugSessionPolicy::MergeIdentity(const Context& a, const Context& b, Context& staged, std::string& error) {
    return MergeEpochIdentity(a, b, staged, error);
}
bool DebugSessionPolicy::Apply(const State& baseline, const Update& update, State& state, std::string& error) {
    return ApplyUpdate(baseline, update, state, error);
}
} // namespace monitor::synchronization
