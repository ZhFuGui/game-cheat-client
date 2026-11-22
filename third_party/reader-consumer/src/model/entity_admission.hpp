#pragma once
#include "model/frame.hpp"
#include "model/entity_collection.hpp"
namespace monitor::model {
bool CurrentSceneSlot(const Snapshot&, const SceneActor&) noexcept;
bool CurrentVehicleClassification(const Snapshot&, const VehicleSample&);
bool CurrentPickupClassification(const Snapshot&, const PickupSample&);
bool CurrentCreatureClassification(const Snapshot&, const CreatureSample&);
bool CurrentContainerClassification(const Snapshot&, const ContainerSample&);
bool CurrentGroundMember(const Snapshot&, const ContainerSample&, const GroundItemMember&);
EntityCollection CollectEntities(const Snapshot&);
// 值模型边界检查；不重新读取游戏，也不依赖编码或传输。
EntityCollection ValidateAndAssignIds(const Snapshot&, const EntityCollection&);
void ValidatePrepared(const Snapshot&, EntityCollection&);
} // namespace monitor::model
