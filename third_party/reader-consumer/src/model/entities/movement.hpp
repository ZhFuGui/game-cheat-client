#pragma once
#include "model/entities/world_velocity.hpp"
#include "model/entities/observation.hpp"
#include "model/current_facts.hpp"

namespace monitor {
// 同一批人物运动组件的真实速度；合法零向量保留，失败清空，不沿用历史动态值。
struct CharacterMovementSample {
    OwnedObservation observation;
    std::optional<Vec3> velocity;
    std::optional<InstanceIdentity> component;
    std::optional<u64> raw_component;
    std::shared_ptr<const WorldVelocitySample> world;
};
inline OwnedObservation MovementObservation(const CharacterMovementSample& sample) {
    auto out = sample.observation;
    const bool raw = sample.velocity && sample.observation.value_status;
    const bool world = sample.world && sample.world->velocity && sample.world->observation.value_status;
    if (raw || world) {
        out.latest_status = raw && world ? OwnedReadStatus::Available : OwnedReadStatus::Partial;
        out.value_status = out.latest_status;
        if (!out.issue && sample.world)
            out.issue = sample.world->observation.issue;
    } else
        out.value_status.reset();
    return out;
}
} // namespace monitor
