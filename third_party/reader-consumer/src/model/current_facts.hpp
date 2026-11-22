#pragma once
#include "model/primitives.hpp"
#include "memory/issue.hpp"
#include <optional>
#include <vector>

namespace monitor {
// 采样区间属于本次真实读取；关闭核验时间独立记录，不代替动态值的采样时间。
struct SampleInterval {
    Clock::time_point started{}, finished{};
};
enum class EntityKind : unsigned { Unknown, Character, Vehicle, Pickup, Creature, Container };
enum class PropertyGroup : unsigned {
    Identity,
    Spatial,
    CharacterState,
    CharacterName,
    VehicleState,
    VehicleSeats,
    VehicleName,
    VehicleModel,
    PickupOwner,
    PickupStack,
    PickupDefinition,
    // 分类是已有身份/定义读取步骤的解释输出，不代表新增内存读取任务。
    VehicleClassification,
    PickupClassification,
    CreatureClassification,
    PickupItemClassification,
    ContainerClassification,
    ContainerContents,
    CharacterSkeleton,
    CharacterMovement,
    Count
};
enum class FactStatus : unsigned { Available, Partial, ReadFailed, DependencyBlocked, NotAttempted, NotImplemented };
struct GroupFact {
    PropertyGroup group = PropertyGroup::Identity;
    FactStatus status = FactStatus::NotAttempted;
    // 各组的字段位定义在对应实体模型；位为 0 表示本次缺失，而非沿用基线。
    u64 present_fields = 0;
    SampleInterval sample;
    Issue issue;
};
// 上下文字段与实体属性一样保存真实来源；缓存值的采样区间不因复用而更新。
enum class ContextFactGroup : unsigned { World, LocalControl, Origin };
enum class ContextSourceKind : unsigned { Memory, Interpretation };
struct ContextFieldSource {
    unsigned field = 0; // 对应 fields::context_* 的字段位序号，从 1 开始。
    SampleInterval sample;
    u64 capture_sequence = 0;
    ContextSourceKind source = ContextSourceKind::Memory;
    bool reused = false;
};
struct ContextFact {
    ContextFactGroup group = ContextFactGroup::World;
    FactStatus status = FactStatus::NotAttempted;
    u64 present_fields = 0;
    std::vector<ContextFieldSource> field_sources;
    // 关闭身份/所属链/解释依据的实际核验窗口，不能替代各字段 source.sample。
    SampleInterval verified;
    Issue issue;
};
struct InstanceIdentity {
    u64 actor_address = 0;
    i32 internal_index = 0;
    u32 serial_number = 0;
    bool operator==(const InstanceIdentity&) const = default;
};
enum class AdmissionState : unsigned { Admitted, Unadmitted, WithdrawnFromScope, IdentityChanged, Retired };
// 只保存类型已确认的尝试和失败地址的终态；正常未知 Actor 不生成虚构属性。
// identity 是采样到的实例；仅 Admitted 表示它通过本帧全部身份/来源关闭核验。
struct ActorFact {
    u64 actor_address = 0;
    std::size_t first_source_slot = 0;
    EntityKind kind = EntityKind::Unknown;
    AdmissionState admission = AdmissionState::Unadmitted;
    std::optional<InstanceIdentity> identity;
    std::optional<InstanceIdentity> replacement;
    bool source_present_at_close = false;
    bool lifecycle_source_verified = false;
    // 保留真实 GU item 观察与关闭区间；不得由批次开头或较晚的尾核验时间代替。
    SampleInterval lifecycle_observed, lifecycle_verified;
    Clock::time_point verified_finished{};
    std::vector<GroupFact> groups;
    Issue issue;
};
struct CurrentFacts {
    u64 capture_sequence = 0;
    bool frame_accepted = false;
    bool source_open_complete = false, source_close_complete = false;
    u64 source_level = 0, opening_container = 0, closing_container = 0;
    SampleInterval opening_source, closing_source;
    // 最终成员复制用于已知旧实体的来源退出判断，不能将 typed 集合差集当销毁。
    std::vector<u64> closing_source_slots;
    std::vector<ContextFact> context_groups;
    std::vector<ActorFact> actors;
    Issue frame_issue;
};
} // namespace monitor
