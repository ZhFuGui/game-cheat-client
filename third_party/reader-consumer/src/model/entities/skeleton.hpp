#pragma once
#include "model/entities/observation.hpp"
#include "model/current_facts.hpp"
#include "model/geometry.hpp"
#include <vector>

namespace monitor {
struct BoneSample {
    std::optional<u32> semantic_role; // BodyBoneNames 中的0..19语义角色，与原生骨索引分离。
    u32 index = 0;
    std::optional<i32> parent_index; // -1 是根骨骼；缺失表示未获得可靠参考骨架。
    std::optional<ObjectName> name;
    i32 pose_index = -1; // 主姿态映射后的真实下标；-1 表示该骨骼没有姿态来源。
    std::optional<Transform> component_transform;
    std::optional<Vec3> world_position; // 与本批原点关联的 UE 世界坐标，未加 WorldOrigin。
};
struct SkeletonSource {
    InstanceIdentity mesh, skeletal_mesh, pose_component;
    u32 read_buffer_index = 0, bone_count = 0, pose_transform_count = 0;
    Transform component_to_world;
    WorldOrigin origin;
};
struct CharacterSkeletonSample {
    OwnedObservation observation;
    std::optional<SampleInterval> pose_sample; // 本批动态姿态读取/其必要来源关闭的真实区间，不含布局准备等待。
    std::optional<SkeletonSource> source;
    std::vector<BoneSample> bones; // 仅已确认人体主干，按真实索引递增；不读取或发送辅助骨姿态。
};
} // namespace monitor
