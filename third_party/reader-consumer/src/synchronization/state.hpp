#pragma once
#include "synchronization/instance_registry.hpp"
#include "model/entities/observation.hpp"
#include "model/category.hpp"
#include "synchronization/publication.hpp"
#include <array>
#include <map>
#include <set>
#include <variant>
#include <vector>

namespace monitor::synchronization {
// 差分比较语义值，绝不比较 protobuf 字节、读取时戳或内存填充字节。
struct Value {
    using List = std::vector<Value>;
    using Object = std::map<unsigned, Value>;
    std::variant<bool, std::int64_t, u64, double, std::string, List, Object> data;
    Value() : data(false) {}
    template <class T> explicit Value(T value) : data(std::move(value)) {}
    bool operator==(const Value&) const = default;
};
struct GroupState {
    FactStatus status = FactStatus::NotAttempted;
    std::map<unsigned, Value> fields;
    SampleInterval source_sample, verified_sample;
    u64 source_capture_sequence = 0, verified_capture_sequence = 0;
    // 路径中普通值为对象字段号，最高位为 1 的值为零起点集合索引。
    // 每个嵌套样本使用其真实观察记录，父组成功不能替它刷新。
    struct NestedSample {
        u64 capture_sequence = 0;
        SampleInterval source, attempt;
        OwnedReadStatus latest_status = OwnedReadStatus::NotAttempted;
        std::optional<OwnedReadStatus> value_status;
    };
    std::map<std::vector<unsigned>, NestedSample> nested_samples;
    struct FieldSource {
        u64 capture_sequence = 0;
        SampleInterval sample;
        bool reused = false;
        unsigned origin = 0; // 1=内存，2=实际解释，3=开帧冻结的配置。
        DataCategory source_category = DataCategory::None;
    };
    std::map<unsigned, FieldSource> field_sources;
};
struct KindContract {
    std::set<PropertyGroup> enabled;
    bool operator==(const KindContract&) const = default;
};
using GroupContract = std::map<EntityKind, KindContract>;
GroupContract CanonicalContract();
bool ValidateContract(const GroupContract&, std::string& error);
// 网络枚举存在 UNSPECIFIED=0，C++ Identity/Available 从 0 起；必须逐值映射。
std::optional<PropertyGroup> PropertyGroupFromWire(unsigned) noexcept;
std::optional<FactStatus> FactStatusFromWire(unsigned) noexcept;
unsigned PropertyGroupToWire(PropertyGroup) noexcept;
unsigned FactStatusToWire(FactStatus) noexcept;
struct KindSelection {
    EntityKind kind;
    std::vector<PropertyGroup> enabled;
};
bool MakeContract(const std::vector<KindSelection>&, GroupContract&, std::string& error);
// Local专属组统一编码在FrameContext.local；普通Actor批次只含四个公共上下文组。
enum class ContextGroup : unsigned {
    World = 1,
    Map = 2,
    Coordinates = 3,
    LocalControl = 4,
    Pacing = 5,
    ViewSource = 6,
    ViewPose = 7,
    ViewTarget = 8,
    LocalWeapon = 9,
    LocalSpectators = 10,
    LocalControllerRotation = 11,
    LocalNativeHolding = 12,
    LocalNativeSway = 13,
    LocalAimBasis = 14
};
using FrameContext = std::map<ContextGroup, GroupState>;
// typed Local 的唯一组映射；可选组只在本批实际提供时计数，不以字段掩码猜测能力。
struct LocalContextBinding {
    ContextGroup group;
    unsigned field;
    bool in_camera, required;
};
inline constexpr std::array LocalContextBindings{
    LocalContextBinding{ContextGroup::LocalControl, 1, false, true},
    LocalContextBinding{ContextGroup::ViewSource, 1, true, true},
    LocalContextBinding{ContextGroup::ViewPose, 2, true, true},
    LocalContextBinding{ContextGroup::ViewTarget, 3, true, true},
    LocalContextBinding{ContextGroup::LocalWeapon, 3, false, true},
    LocalContextBinding{ContextGroup::LocalSpectators, 4, false, true},
    LocalContextBinding{ContextGroup::LocalControllerRotation, 5, false, false},
    LocalContextBinding{ContextGroup::LocalNativeHolding, 6, false, false},
    LocalContextBinding{ContextGroup::LocalNativeSway, 7, false, false},
    LocalContextBinding{ContextGroup::LocalAimBasis, 8, false, false}};
constexpr const LocalContextBinding* LocalContextBindingFor(ContextGroup group) noexcept {
    for (const auto& binding : LocalContextBindings)
        if (binding.group == group)
            return &binding;
    return nullptr;
}
constexpr bool IsLocalContextGroup(ContextGroup group) noexcept {
    return LocalContextBindingFor(group) != nullptr;
}
inline std::size_t ExpectedContextGroups(const FrameContext& context, bool local) noexcept {
    std::size_t count = 4;
    if (local)
        for (const auto& binding : LocalContextBindings)
            count += binding.required || context.contains(binding.group);
    return count;
}
u64 AllowedContextFields(ContextGroup) noexcept;
struct EntityState {
    u64 id = 0;
    InstanceKey key;
    std::map<PropertyGroup, GroupState> groups;
};
struct SourceScope {
    bool opening_complete = false, closing_complete = false;
    u64 level = 0, opening_slots = 0, closing_slots = 0;
    u64 opening_container = 0, closing_container = 0;
    SampleInterval opening, closing;
};
enum class RemovalReason { Unknown, ReadFailure, WithdrawnFromScope, InstanceRetired, InstanceReplaced };
struct BatchObservation {
    DataCategory category = DataCategory::None;
    u64 directory_generation = 0, classification_generation = 0;
    SampleInterval directory_sample, classification_sample;
    u64 membership_generation = 0;
    SampleInterval membership_sample;
    u64 planned = 0, succeeded = 0, read_failed = 0, dependency_blocked = 0, not_executed = 0;
};
struct State : PublicationState {
    SourceScope source;
    BatchObservation batch;
    GroupContract contract = CanonicalContract();
    FrameContext context;
    std::map<u64, EntityState> entities;
    // 本帧源事实驱动的撤销理由；不由 typed 集合差集推导销毁。
    std::map<u64, RemovalReason> removals;
    // 本帧最后一次目标数据读取及必要读取核验结束；不包含之后的DTO/事实整理。
    DataCategory Category() const noexcept override { return batch.category; }
    std::size_t EntityCount() const noexcept override { return entities.size(); }
    bool SameEntity(u64 id, const PublicationState& other) const noexcept override {
        const auto* state = dynamic_cast<const State*>(&other);
        if (!state) return false;
        const auto a = entities.find(id), b = state->entities.find(id);
        return a != entities.end() && b != state->entities.end() && a->second.key == b->second.key;
    }
};
u64 FieldMask(const std::map<unsigned, Value>& fields);
u64 AllowedFields(PropertyGroup) noexcept;
bool GroupAllowed(EntityKind, PropertyGroup) noexcept;
bool ValidateState(const State&, std::string& error);
bool SameState(const State&, const State&);
// 只比较本代次已确认的 World/Level/坐标身份；当前缺失不代表旧确认被撤销。
bool SameEpochContext(const FrameContext& confirmed, const FrameContext& current, std::string& error);
// 非输出身份栅栏：仅限制上下文身份，不提供读取地址或动态值；暂时未知不补回当前帧。
// 冲突或分配失败不修改 staged，成功后只包含 World/Level/坐标身份的语义值。
bool MergeEpochIdentity(const FrameContext& confirmed, const FrameContext& current, FrameContext& staged,
                        std::string& error);
bool SameEpochContext(const State&, const FrameContext&, std::string& error);
} // namespace monitor::synchronization
