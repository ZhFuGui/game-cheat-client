#pragma once
#include "model/primitives.hpp"
#include "model/entities/observation.hpp"
#include <optional>

namespace monitor {
// 与实体种类无关的原始字段证据；不通过装备或物品模型间接提供人物依赖。
struct OwnedRawField {
    std::string field_name;
    u64 owner = 0;
    std::optional<u64> offset;
    std::string bytes;
    OwnedObservation observation;
};
} // namespace monitor
