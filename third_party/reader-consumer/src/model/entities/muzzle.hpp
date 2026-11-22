#pragma once
#include "model/entities/skeleton.hpp"
#include <memory>

namespace monitor {
// 真实枪口挂点的几何变换；不是归零、散布或弹道修正之后的发射方向。
enum class MuzzleBinding : u32 { AssetSocket = 1, SkeletonSocket = 2, Bone = 3 };
struct WeaponMuzzleSample {
    OwnedObservation observation;
    std::optional<SkeletonSource> source;
    std::optional<InstanceIdentity> socket;
    std::optional<ObjectName> attach_name, bone_name;
    std::optional<u32> bone_index, pose_index;
    std::optional<MuzzleBinding> binding;
    std::optional<Transform> relative_transform, bone_component_transform, world_transform;
};
} // namespace monitor
