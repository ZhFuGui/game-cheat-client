#pragma once
#include "model/session.hpp"
#include "model/category.hpp"
#include "model/current_facts.hpp"
#include "model/local.hpp"
#include "model/entities/character.hpp"
#include "model/entities/vehicle.hpp"
#include "model/entities/pickup.hpp"
#include "model/entities/creature.hpp"
#include "model/entities/container.hpp"
#include "memory/read_stats.hpp"
#include "model/capture_metrics.hpp"
#include <array>
#include <map>
#include <memory>
namespace monitor::model {
struct EntityCollection;
}
namespace monitor {
struct SceneActor {
    u64 actor = 0, raw = 0;
    u64 sample_capture_sequence = 0; // Original observation attempt; never refreshed by export.
    i32 index = -1;
    u64 source_level = 0, source_container = 0; // 当前主 Level 及其 Actor 数组；index 是原始槽位。
    Clock::time_point observed{};
    Clock::time_point observed_finished{};
    Issue acquisition_issue;
    std::string availability = "NOT_SAMPLED";
};
struct Snapshot {
    DataCategory category = DataCategory::None;
    SubscriptionProfile subscription_profile = SubscriptionProfile::Consumer;
    SubscriptionFeatures subscription_features = 0;
    u64 plan_revision = 0;
    u64 directory_generation = 0, classification_generation = 0;
    // 内部成员内容版本；相同名单的重复观察不迫使已读完的批次无限重整。
    u64 directory_membership_revision = 0, membership_revision = 0;
    SampleInterval directory_sample, classification_sample;
    u64 membership_generation = 0;
    SampleInterval membership_sample;
    Session session;
    std::optional<capture::FrameMetrics> pipeline;
    std::shared_ptr<const model::EntityCollection> prepared_entities;
    std::shared_ptr<const CurrentFacts> current_facts;
    std::optional<LocalSample> local; // 独立本地客户端状态，相机与身体属性各有终态。
    Clock::time_point started = Clock::now(), finished = Clock::now();
    // 本批最后一次数据读取及必要核验完成，包括最终进程实例与地址空间核验。
    // 未到达时缺失；此前 DTO 整理和尾核验等待计入区间，之后的 CPU 提交不计入。
    std::optional<Clock::time_point> read_finished;
    Clock::time_point core_finished{}; // 当前属性与来源关闭完成；编码和传输不改写此时刻。
    u64 sequence = 0;
    bool complete = false;
    Issue fatal, reference_issue, local_control_issue;
    u64 world_state = 0, world = 0, game_state_raw = 0, game_state = 0;
    bool world_cache_used = false, world_root_unverified = false;
    bool world_refresh_attempted = false, world_refresh_succeeded = false, world_refresh_failed = false;
    bool world_cache_invalidated = false;
    std::int64_t world_cache_age_ms = -1; // Since the last successful raw-root read.
    u64 game_instance_raw = 0, game_instance = 0, level_raw = 0, level = 0;
    u64 local_player = 0, controller = 0, self_pawn = 0;
    u64 local_player_state = 0, local_character = 0; // 当前核验的人物身份与实际被控制 Pawn 独立。
    std::optional<InstanceIdentity> local_character_identity, self_pawn_identity;
    EntityKind self_pawn_kind = EntityKind::Unknown;
    std::optional<Vec3> reference;
    std::optional<WorldOrigin> origin; // 本帧所有空间属性使用且最终关闭核验的同一原点。
    Issue origin_issue, world_name_issue;
    ObjectName world_name;
    NameCaptureStatus world_name_status = NameCaptureStatus::NotAttempted;
    u64 name_world = 0; // Independent metadata identity; may differ from a rejected core capture.
    bool name_root_unverified = true;
    Clock::time_point world_name_observed{};
    Clock::time_point world_name_finished{};
    u64 world_name_capture_sequence = 0;
    ArrayHeader players_header{}, local_players_header{};
    std::vector<u64> player_roster_slots; // Exact copied slots, including null/invalid/duplicate values.
    std::optional<float> time_seconds;
    std::optional<i32> alive_teams_raw;
    std::vector<Player> players, unmatched;
    std::size_t actor_objects = 0, matched = 0, positioned = 0, actor_failures = 0;
    std::size_t backend_calls = 0, failed_attempts = 0;
    std::optional<Issue> first_actor_failure;
    std::vector<std::string> trace;
    std::map<std::string, ReadStageStats> read_stats;
    std::size_t player_slots_unusable = 0;
    // 已核验类型的来源关联；完整原始范围在 scene_actor_slots，属性各自独立定时。
    std::vector<SceneActor> scene_actors;
    std::vector<u64> scene_actor_slots; // 独立闭合的主 Level 原始 Actor 槽位，不以可选属性替代来源证明。
    bool scene_list_valid = false;
    std::size_t scene_attempted = 0;
    u64 scene_header_address = 0, scene_array_state = 0;
    ArrayHeader scene_header{};
    Clock::time_point scene_list_started{}, scene_list_finished{};
    std::vector<std::shared_ptr<const VehicleSample>> vehicles;
    std::vector<std::shared_ptr<const PickupSample>> pickups;
    std::vector<std::shared_ptr<const CreatureSample>> creatures;
    std::vector<std::shared_ptr<const ContainerSample>> containers;
};
} // namespace monitor
