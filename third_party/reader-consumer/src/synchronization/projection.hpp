#pragma once
#include "model/frame.hpp"
#include "synchronization/state.hpp"

namespace monitor::synchronization {
struct ProjectionOptions {
    GroupContract contract = CanonicalContract();
    std::string coordinate_convention;
    std::optional<double> verified_meters_per_unit;
    std::string coordinate_rules_revision;
    std::string reader_instance_id;
};
// 只消费采集器的 CurrentFacts 和本帧不可变模型；不读取内存，不请求分类或重采样。
State ProjectCurrent(const Snapshot&, u64 epoch, u64 revision, InstanceRegistry&, const ProjectionOptions& = {});
} // namespace monitor::synchronization
