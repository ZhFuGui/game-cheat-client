#pragma once
#include "model/entities/item.hpp"
#include "model/current_facts.hpp"

namespace monitor {
// 原生 Vehicle 类型已通过实际构造函数、接口及 getter 核验；未知子类别不是未知实体类型。
enum class VehicleKind : unsigned { Unknown = 0, Wheeled = 1, Boat = 2, Aircraft = 3, Bicycle = 4, Special = 5 };
struct VehicleStateSample {
    OwnedObservation observation;
    std::optional<float> health, max_health, fuel, max_fuel;
    std::optional<bool> engine_running, destroyed, is_dying, is_broken;
    std::optional<u64> common_component_address, raw_common_component_value;
    std::vector<OwnedRawField> raw_fields;
};
struct VehicleSpatialSample {
    OwnedObservation observation;
    std::optional<Vec3> position;
    std::optional<WorldOrigin> origin;
    std::optional<u64> root_component_address, raw_root_component_value;
    std::vector<OwnedRawField> raw_fields;
};
enum class SeatOccupancy : unsigned { Unknown = 0, Empty = 1, Occupied = 2 };
struct VehicleSeatRider {
    OwnedObservation observation;
    // Entity ID 由发布端在本帧最终人物准入后解析，采集端不猜测 ID。
    std::optional<u64> character_entity_id, raw_pawn_value, pawn_address;
    std::optional<i32> source_slot_index;
    // 本批实际读取并关闭核验的乘员生命周期，用于跨类别身份引用。
    std::optional<InstanceIdentity> instance_identity;
};
struct VehicleSeatRidersSample {
    OwnedObservation observation;
    OwnedCoverage coverage = OwnedCoverage::NotAttempted;
    std::optional<u32> declared_slots, active_members;
    std::vector<VehicleSeatRider> riders;
};
struct VehicleSeat {
    OwnedObservation observation;
    std::optional<i32> seat_index;
    std::optional<u64> component_address;
    SeatOccupancy occupancy = SeatOccupancy::Unknown;
    std::optional<VehicleSeatRidersSample> riders;
};
struct VehicleSeatsSample {
    OwnedObservation observation;
    OwnedCoverage coverage = OwnedCoverage::NotAttempted;
    std::optional<u32> declared_slots, occupied_slots;
    std::vector<VehicleSeat> seats;
};
struct VehicleModelSample {
    OwnedObservation observation;
    std::optional<u64> class_address, class_pointer_raw;
    ObjectName class_name;
    std::optional<std::string> model_key;
    std::string interpretation_revision;
};
struct VehicleSample {
    u64 actor = 0, vtable = 0, raw_name = 0;
    VehicleKind kind = VehicleKind::Unknown;
    std::string classification_rule_id;
    OwnedObservation classification;
    std::vector<OwnedRawField> classification_raw_fields;
    i32 source_index = -1;
    u64 source_container = 0, source_level = 0;
    ObjectName name;
    OwnedObservation name_observation;
    std::optional<VehicleModelSample> model;
    VehicleSpatialSample spatial;
    std::optional<VehicleStateSample> state;
    std::optional<VehicleSeatsSample> seats;
    // 未实现的属性不从人物名单、历史引用或名称推断。
};
} // namespace monitor
