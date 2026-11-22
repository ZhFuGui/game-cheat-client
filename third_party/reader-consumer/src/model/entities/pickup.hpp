#pragma once
#include "model/entities/item.hpp"

namespace monitor {
struct PickupSpatialSample {
    OwnedObservation observation;
    std::optional<Vec3> position;
    std::optional<WorldOrigin> origin;
    std::optional<u64> root_component_address, raw_root_component_value;
    std::vector<OwnedRawField> raw_fields;
};

// 只有本帧仍在 Actor 来源中、原生类型及对象身份核验成功的独立拾取 Actor 才能准入。
// 掉落组、其成员组件、UItem、物品定义和人物装备均不作为 PickupSample。
struct PickupSample {
    u64 actor = 0, vtable = 0, raw_name = 0;
    std::string classification_rule_id;
    OwnedObservation classification;
    std::vector<OwnedRawField> classification_raw_fields;
    i32 source_index = -1;
    u64 source_container = 0, source_level = 0;
    ObjectName name;
    ObjectName instance_name_raw;
    OwnedObservation name_observation;
    std::optional<i32> object_internal_index;
    std::optional<u32> object_serial_number;
    PickupSpatialSample spatial;
    std::optional<OwnedItemEntry> item;
};
} // namespace monitor
