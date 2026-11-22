#pragma once
#include "model/current_facts.hpp"
#include "model/entities/observation.hpp"
#include "model/primitives.hpp"
#include <memory>

namespace monitor {
// 与当前版本 Actor::GetVelocity 的分支一致，不做跨帧位置差分或消费端载具速度叠加。
enum class WorldVelocitySource : u32 { Movement = 1, Physics = 2, Riding = 3, Based = 4, NativeZero = 5 };
struct WorldVelocitySample {
    OwnedObservation observation;
    std::optional<Vec3> velocity;
    std::optional<WorldVelocitySource> source;
    std::optional<InstanceIdentity> actor, component, carrier;
};
} // namespace monitor
