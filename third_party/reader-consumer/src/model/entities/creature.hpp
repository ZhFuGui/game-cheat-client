#pragma once
#include "model/entities/item.hpp"

namespace monitor {
enum class CreatureSpecies : unsigned { Unknown = 0, Bear = 1 };
struct CreatureSpatialSample {
    OwnedObservation observation;
    std::optional<Vec3> position;
    std::optional<WorldOrigin> origin;
    std::optional<u64> root_component_address, raw_root_component_value;
    std::vector<OwnedRawField> raw_fields;
};
// 怪物角色与普通人物分开准入；PlayerState 仅提供经核验的原生类型标记，
// 不要求它属于人物 PlayerArray，也不伪造队伍、名字或人机属性。
struct CreatureSample {
    u64 actor = 0, vtable = 0, raw_name = 0;
    ObjectName instance_name_raw;
    u64 source_level = 0, source_container = 0;
    i32 source_index = -1;
    std::optional<u32> object_internal_index, object_serial_number;
    u64 player_state = 0, player_state_vtable = 0;
    std::optional<u8> character_type_raw;
    CreatureSpecies species = CreatureSpecies::Unknown;
    OwnedObservation classification;
    std::vector<OwnedRawField> classification_raw_fields;
    std::string interpretation_revision;
    CreatureSpatialSample spatial;
};
} // namespace monitor
