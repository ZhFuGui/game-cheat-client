#pragma once
#include "synchronization/consumer/state.hpp"

namespace monitor::synchronization::consumer {
// 当前业务终态与来源时刻；没有原生身份、关闭证明或历史动态值。
struct GroupRefresh {
    FactStatus status = FactStatus::NotAttempted;
    bool has_value = false;
    u64 source_capture_sequence = 0;
    SampleInterval source;
    std::map<std::vector<unsigned>, SourceTiming> sources;
};
struct EntityRefresh {
    EntityKind kind = EntityKind::Unknown;
    std::map<PropertyGroup, GroupRefresh> groups;
};
struct Update {
    u64 epoch = 0, baseline_generation = 0, fence = 0, base_revision = 0, revision = 0, capture_sequence = 0;
    bool accepted = false;
    SampleInterval capture;
    std::optional<Clock::time_point> read_finished;
    DataCategory category = DataCategory::None;
    SubscriptionFeatures features = 0;
    // context.local_groups 为空；Local真实终态独立位于local_current。
    Context context;
    bool replace_context_values = false;
    std::map<u64, EntityRefresh> current;
    std::map<ContextGroup, GroupRefresh> local_current;
    std::map<u64, Entity> additions;
    std::map<u64, RemovalReason> removals;
    std::map<std::pair<u64, PropertyGroup>, std::optional<GroupValue>> changes;
    std::map<ContextGroup, std::optional<GroupValue>> local_changes;
    std::optional<u64> send_started_qpc;
};
Update BuildUpdate(const State& baseline, const State& current, u64 generation, u64 fence);
// 基线已由同一共享Session内核完整验证并独占持有；当前状态仍须完整验证。
Update BuildAcknowledgedUpdate(const State& baseline, const State& current, u64 generation, u64 fence);
bool ApplyUpdate(const State& baseline, const Update&, State& candidate, std::string& error);
bool SameContextValues(const Context&, const Context&) noexcept;
} // namespace monitor::synchronization::consumer
