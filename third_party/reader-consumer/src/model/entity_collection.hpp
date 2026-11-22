#pragma once
#include "model/entities/character.hpp"
#include "model/entities/vehicle.hpp"
#include "model/entities/pickup.hpp"
#include "model/entities/creature.hpp"
#include "model/entities/container.hpp"
#include <memory>
#include <vector>
namespace monitor {
struct Snapshot;
}
namespace monitor::model {

enum class SourceKind : unsigned { SceneArray = 1, PlayerRoster = 2 };

// 来源槽位属于本批实际借用的完整目录；属性在本批独立采样。
struct EntitySource {
    SourceKind kind = SourceKind::PlayerRoster;
    u64 container_address = 0, level_address = 0;
    std::optional<i32> slot_index;
    u64 raw_value = 0, capture_sequence = 0; // SceneArray 为目录代次，PlayerRoster 为活体批次序号。
    u64 started_ns = 0, finished_ns = 0;
};

struct RawField {
    std::string field_name;
    u64 owner = 0;
    std::optional<u64> offset;
    std::string bytes;
    u64 sample_capture_sequence = 0;
    u64 observed_started_monotonic_ns = 0, observed_finished_monotonic_ns = 0;
    bool partial = false;
};

// 身体由当前主 Level 原生类型和 Pawn 生命周期核验准入。
// PlayerState 为可选关联；非空时仍须核验，缺失不撤销独立的身体属性。
struct Character {
    u64 entity_id = 0, actor_id = 0, player_state_id = 0, root_component_id = 0;
    bool actor_identity_verified = false;
    // 只转发本帧最终关闭时已核验的 GUObjects 生命周期证据，不沿用历史值。
    std::optional<i32> object_internal_index;
    std::optional<u32> object_serial_number;
    std::optional<i32> roster_index, team_id;
    std::optional<u32> dbno_raw, killed_raw;
    std::optional<Vec3> position, rejected_position_raw;
    u64 rejected_position_root = 0;
    std::optional<WorldOrigin> origin;
    std::optional<std::string> display_name;
    std::string classification_rule_id = "verified-playerstate-pawn";
    u64 sample_capture_sequence = 0;
    u64 observed_started_monotonic_ns = 0, observed_finished_monotonic_ns = 0;
    Issue acquisition_issue;
    std::vector<EntitySource> sources;
    std::vector<RawField> raw_fields;
    std::shared_ptr<const BotClassificationSample> bot_classification;
    std::shared_ptr<const CharacterTypeSample> character_type;
    std::shared_ptr<const CharacterSkeletonSample> skeleton;
    std::shared_ptr<const CharacterMovementSample> movement;
};

// 类型、来源与生命周期核验独立于可选空间和状态字段。
struct Vehicle {
    u64 entity_id = 0;
    std::shared_ptr<const VehicleSample> sample;
    std::vector<EntitySource> sources;
    std::optional<i32> object_internal_index;
    std::optional<u32> object_serial_number;
};

// 独立拾取 Actor 的准入不依赖可选物品名称、数量或位置是否可读。
struct Pickup {
    u64 entity_id = 0;
    std::shared_ptr<const PickupSample> sample;
    std::vector<EntitySource> sources;
};

// 生物的原生类型和怪物 PlayerState 关联独立核验，不套用人物名单。
struct Creature {
    u64 entity_id = 0;
    std::shared_ptr<const CreatureSample> sample;
    std::vector<EntitySource> sources;
};

struct Container {
    u64 entity_id = 0;
    std::shared_ptr<const ContainerSample> sample;
    std::vector<EntitySource> sources;
};

struct ClassificationStats {
    u64 examined_source_slots = 0, admitted_source_slots = 0;
    u64 skipped_invalid_slots = 0, skipped_unclassified_slots = 0;
    u64 skipped_conflicting_slots = 0;
    // A subset of admitted_source_slots, not an additional skip bucket. Each
    // second or later admitted occurrence of the same object counts here.
    u64 duplicate_source_slots = 0;
};

struct SkippedIssue {
    Issue issue;
    u64 sample_capture_sequence = 0;
    u64 observed_started_monotonic_ns = 0, observed_finished_monotonic_ns = 0;
};

struct EntityCollection {
    std::vector<Character> characters;
    std::vector<Vehicle> vehicles;
    std::vector<Pickup> pickups;
    std::vector<Creature> creatures;
    std::vector<Container> containers;
    ClassificationStats classification;
    std::vector<SkippedIssue> issues;
};
EntityCollection CollectEntities(const Snapshot&);

} // namespace monitor::model
