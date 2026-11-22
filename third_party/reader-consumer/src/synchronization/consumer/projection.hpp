#pragma once
#include "synchronization/consumer/state.hpp"
#include "synchronization/projection.hpp"
namespace monitor::synchronization::consumer {
// 仅投影阶段持有原生证据；不进入消费State或编码器。
struct IdentityUse {
    InstanceKey key;
    SampleInterval interval;
    bool direct = false;
};
using TokenKey = std::vector<u64>;
struct ProjectionResult {
    State state;
    std::map<u64, IdentityUse> identities;
    std::map<u64, InstanceKey> entity_keys;
    std::map<u64, TokenKey> tokens;
};
ProjectionResult ProjectCurrent(const Snapshot&, u64 epoch, u64 revision, InstanceRegistry&,
                                const ProjectionOptions& options = {});
} // namespace monitor::synchronization::consumer
