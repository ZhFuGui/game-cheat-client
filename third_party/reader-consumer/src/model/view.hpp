#pragma once
#include "model/current_facts.hpp"

namespace monitor {
// Unreal FRotator 的顺序与单位；不能用 Vec3 的 xyz 名称隐去俯仰、偏航和滚转语义。
struct ViewRotation {
    float pitch = 0, yaw = 0, roll = 0; // 度。
};
static_assert(sizeof(ViewRotation) == 12);

// 本地客户端最终视点；死亡、观战与骑乘不要求存在本地身体或 PlayerState。
struct ViewSample {
    u64 controller = 0, camera_manager = 0;
    std::optional<InstanceIdentity> controller_identity, manager_identity, target_identity;
    EntityKind target_kind = EntityKind::Unknown;
    // 成功读到 0 表示明确无目标；缺失表示该字段没有成功读取，不能借用历史目标。
    std::optional<u64> target_address;
    std::optional<u64> current_target_address, pending_target_address; // Current/Pending 原始目标，混合 POV 不归属单一目标。
    std::optional<Vec3> location;
    std::optional<ViewRotation> rotation;
    std::optional<float> fov_degrees;
    std::optional<float> cached_fov_degrees, locked_fov_degrees; // 原生 getter 的两项来源，不改变锁值的零/负语义。
    std::optional<float> camera_cache_time_seconds; // 游戏侧相机缓存时间；不是 Reader 帧有效期。
    std::optional<WorldOrigin> origin;
    // 来源、最终视点和目标分别保留真实采样时间，关闭核验不能刷新动态值时间。
    SampleInterval controller_sample, manager_sample, camera_sample, target_sample;
    SampleInterval verified, controller_verified, manager_verified, target_verified;
    FactStatus source_status = FactStatus::NotAttempted, pose_status = FactStatus::NotAttempted,
               target_status = FactStatus::NotAttempted;
    bool source_verified = false;
    Issue issue, pose_issue, target_issue;
};
} // namespace monitor
