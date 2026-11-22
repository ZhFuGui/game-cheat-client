#pragma once
#include "model/entities/pickup.hpp"

namespace monitor {
// 目前仅实现原生地面掉落组；其组件成员不是独立 Actor。
enum class ContainerKind : unsigned { Unknown = 0, LootGroup = 1, DeathBox = 2, Airdrop = 3, Other = 4 };
enum class GroundSlotStatus : unsigned {
    Vacant = 1, Unclassified = 2, ReadFailed = 3, Invalid = 4, Duplicate = 5, Withdrawn = 6, Admitted = 7
};
struct GroundCollectionSource {
    u64 set_address = 0, data_address = 0, allocation_bitmap_address = 0;
    i32 declared_slots = 0, capacity = 0, allocation_bits = 0, allocation_capacity = 0;
    i32 first_free_index = -1, free_slots = 0;
};
struct GroundSlotRecord {
    i32 slot_index = -1;
    bool allocated = false;
    std::optional<u64> raw_component;
    GroundSlotStatus status = GroundSlotStatus::ReadFailed;
};
struct GroundItemMember {
    // 归属观察核验 owner、原始源槽位、分配状态和组件生命周期。
    OwnedObservation observation;
    i32 slot_index = -1;
    u64 owner_actor = 0, component_address = 0, raw_component_value = 0;
    std::optional<i32> object_internal_index;
    std::optional<u32> object_serial_number;
    OwnedObservation classification;
    std::optional<u64> native_vtable, class_address;
    std::string classification_rule_id;
    PickupSpatialSample spatial;
    std::optional<OwnedItemEntry> item;
};
struct ContainerContentsSample {
    OwnedObservation observation;
    OwnedObservation opening_observation, closing_observation;
    OwnedCoverage coverage = OwnedCoverage::NotAttempted;
    std::optional<u32> declared_slots, occupied_slots, admitted_members;
    std::optional<GroundCollectionSource> opening_source, closing_source;
    std::vector<GroundSlotRecord> slots;
    std::vector<GroundItemMember> members;
};
struct ContainerSample {
    u64 actor = 0, vtable = 0, raw_name = 0;
    ContainerKind kind = ContainerKind::Unknown;
    std::string classification_rule_id;
    OwnedObservation classification;
    std::vector<OwnedRawField> classification_raw_fields;
    i32 source_index = -1;
    u64 source_container = 0, source_level = 0;
    ObjectName instance_name_raw;
    std::optional<i32> object_internal_index;
    std::optional<u32> object_serial_number;
    std::optional<ContainerContentsSample> contents;
};
} // namespace monitor
