#pragma once
#include "synchronization/state.hpp"

namespace monitor::synchronization {
enum class PatchOperation { Set, Clear, Replace };
struct GroupProof {
    FactStatus status = FactStatus::NotAttempted;
    u64 present_fields = 0;
    SampleInterval verified_sample;
    u64 verified_capture_sequence = 0;
    std::map<std::vector<unsigned>, GroupState::NestedSample> nested_samples;
    // 本批真实属性来源；数值没有变化时也必须刷新这项证据。
    SampleInterval source_sample;
    u64 source_capture_sequence = 0;
};
struct EntityProof {
    InstanceKey key;
    std::map<PropertyGroup, GroupProof> groups;
};
struct PropertyPatch {
    u64 entity_id = 0;
    PropertyGroup group = PropertyGroup::Identity;
    PatchOperation operation = PatchOperation::Set;
    std::map<unsigned, Value> values;
    u64 clear_fields = 0;
    SampleInterval source_sample;
    u64 source_capture_sequence = 0;
};
struct Update {
    u64 epoch = 0, baseline_generation = 0, fence = 0, base_revision = 0, revision = 0, capture_sequence = 0;
    bool accepted = false;
    SampleInterval capture;
    SourceScope source;
    BatchObservation batch;
    FrameContext context;
    std::map<u64, EntityState> additions;
    std::map<u64, RemovalReason> removals;
    std::vector<PropertyPatch> patches;
    // 当前可发布的完整实例/属性证据清单；只有差分值可以省略，核验资格不能隐式继承。
    std::map<u64, EntityProof> proofs;
    std::optional<Clock::time_point> read_finished;
    // 仅属于本次传输尝试，不参与不可变基线、差分属性或源采样时间。
    std::optional<u64> send_started_qpc;
};
Update BuildUpdate(const State& baseline, const State& current, u64 generation, u64 fence);
bool ApplyUpdate(const State& baseline, const Update&, State& candidate, std::string& error);
} // namespace monitor::synchronization
