#pragma once
#include "model/raw_field.hpp"
#include <vector>
namespace monitor {
struct OwnedItemInfo {
    OwnedObservation observation;
    std::optional<u32> item_id;
    std::optional<ObjectName> identifier;
    std::optional<std::string> display_name;
    u32 category = 0, classification_status = 0;
    std::string classification_rule_id, classification_rules_revision;
    // 完整分类结果的独立本次观察；缺失表示没有已实现的分类器输出。
    // 存在且有 value_status 时，零值和空规则字符串是本次实际输出，不能按 truthiness 丢弃。
    std::optional<OwnedObservation> classification;
    std::optional<u64> definition_address;
    std::vector<OwnedRawField> raw_fields;
};

// 实例数量/耐久与物品定义、所属对象分别核验；不使用定义中的默认值代替实例值。
struct OwnedItemStackSample {
    OwnedObservation observation;
    std::optional<i32> quantity;
    std::optional<float> durability, max_durability;
};

// UItem 等源对象只是持有者或场景对象的嵌套数据，不分配顶层实体 ID。
struct OwnedItemEntry {
    OwnedObservation observation;
    std::optional<OwnedItemInfo> item_info;
    std::optional<OwnedItemStackSample> stack;
    std::optional<u64> source_object_address;
    std::optional<i32> slot_index;
    std::vector<OwnedRawField> raw_fields;
};

} // namespace monitor
