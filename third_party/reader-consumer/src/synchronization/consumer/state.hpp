#pragma once
#include "model/subscription.hpp"
#include "model/current_facts.hpp"
#include "model/category.hpp"
#include "synchronization/state.hpp"
#include "synchronization/publication.hpp"
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace monitor::synchronization::consumer {
// 仅业务值与真实采样时间；不保存原生地址、serial、来源槽位、分类规则或证明树。
struct Vector {
    float x = 0, y = 0, z = 0;
    bool operator==(const Vector&) const = default;
};
struct Origin {
    i32 x = 0, y = 0, z = 0;
    bool operator==(const Origin&) const = default;
};
struct Rotation {
    float pitch = 0, yaw = 0, roll = 0;
    bool operator==(const Rotation&) const = default;
};
struct Quaternion {
    float x = 0, y = 0, z = 0, w = 1;
    bool operator==(const Quaternion&) const = default;
};
struct Transform {
    Quaternion rotation;
    Vector translation, scale{1, 1, 1};
    bool operator==(const Transform&) const = default;
};
struct Reference {
    u64 id = 0;
    EntityKind kind = EntityKind::Unknown;
    bool operator==(const Reference&) const = default;
};
struct Spatial {
    std::optional<Vector> position;
    std::optional<Origin> origin;
    bool operator==(const Spatial&) const = default;
};
struct LivingState {
    std::optional<i32> team;
    std::optional<bool> is_bot;
    std::optional<u32> character_type;
    bool operator==(const LivingState&) const = default;
};
struct Name {
    std::optional<std::string> value;
    bool operator==(const Name&) const = default;
};
struct WorldVelocity {
    std::optional<Reference> owner;
    std::optional<Vector> velocity;
    std::optional<u32> source;
    bool operator==(const WorldVelocity&) const = default;
};
struct Movement {
    std::optional<Vector> component_velocity;
    std::optional<WorldVelocity> world;
    bool operator==(const Movement&) const = default;
};
enum class SemanticBone : u32 {
    Pelvis = 1,
    Spine01,
    Spine02,
    Spine03,
    Neck,
    Head,
    ClavicleLeft,
    UpperArmLeft,
    LowerArmLeft,
    HandLeft,
    ClavicleRight,
    UpperArmRight,
    LowerArmRight,
    HandRight,
    ThighLeft,
    CalfLeft,
    FootLeft,
    ThighRight,
    CalfRight,
    FootRight
};
struct Bone {
    SemanticBone role = SemanticBone::Pelvis;
    std::optional<SemanticBone> parent_role;
    std::optional<Vector> world_position;
    bool operator==(const Bone&) const = default;
};
struct Skeleton {
    std::optional<Reference> owner;
    std::optional<u64> layout_token, pose_token;
    std::optional<Origin> origin;
    std::vector<Bone> bones;
    bool operator==(const Skeleton&) const = default;
};
struct VehicleState {
    std::optional<float> health, max_health, fuel, max_fuel;
    std::optional<bool> engine_running, destroyed, is_dying, is_broken;
    bool operator==(const VehicleState&) const = default;
};
struct Seat {
    std::optional<i32> index;
    u32 occupancy = 0, coverage = 0;
    std::vector<Reference> riders;
    bool operator==(const Seat&) const = default;
};
struct Seats {
    u32 coverage = 0;
    std::optional<u32> declared_seats, occupied_seats;
    std::vector<Seat> seats;
    bool operator==(const Seats&) const = default;
};
struct VehicleModel {
    std::optional<std::string> model_key, class_name;
    bool operator==(const VehicleModel&) const = default;
};
struct VehicleType {
    u32 kind = 0;
    bool operator==(const VehicleType&) const = default;
};
struct CreatureType {
    std::optional<u32> character_type;
    u32 species = 0;
    bool operator==(const CreatureType&) const = default;
};
struct ContainerType {
    u32 kind = 0;
    bool operator==(const ContainerType&) const = default;
};
struct ItemDefinition {
    std::optional<u32> item_id;
    std::optional<std::string> identifier, display_name;
    bool operator==(const ItemDefinition&) const = default;
};
struct ItemClassification {
    u32 category = 0, status = 0;
    bool operator==(const ItemClassification&) const = default;
};
struct Stack {
    std::optional<i32> quantity;
    std::optional<float> durability, max_durability;
    bool operator==(const Stack&) const = default;
};
struct GroundMember {
    u64 id = 0;
    std::optional<Spatial> spatial;
    std::optional<ItemDefinition> definition;
    std::optional<Stack> stack;
    std::optional<ItemClassification> classification;
    bool operator==(const GroundMember&) const = default;
};
struct GroundContents {
    u32 coverage = 0;
    std::vector<GroundMember> members;
    bool operator==(const GroundContents&) const = default;
};
struct LocalControl {
    std::optional<u64> controller_token;
    std::optional<Reference> character, controlled;
    bool operator==(const LocalControl&) const = default;
};
struct Camera {
    std::optional<u64> controller_token, source_token;
    std::optional<Vector> position;
    std::optional<Rotation> rotation;
    std::optional<float> fov_degrees, cached_fov_degrees, locked_fov_degrees, cache_time_seconds;
    std::optional<Origin> origin;
    bool operator==(const Camera&) const = default;
};
struct CameraTarget {
    std::optional<bool> has_target, pending_active;
    std::optional<Reference> target;
    bool operator==(const CameraTarget&) const = default;
};
struct Muzzle {
    std::optional<Origin> origin;
    std::optional<std::string> attach_name, bone_name;
    std::optional<u32> binding;
    std::optional<Transform> world_transform;
    bool operator==(const Muzzle&) const = default;
};
struct Recoil {
    std::optional<float> target_pitch, target_yaw, recovery_target;
    bool operator==(const Recoil&) const = default;
};
struct Weapon {
    std::optional<Reference> owner;
    std::optional<u64> weapon_token;
    std::optional<bool> has_current_item;
    std::optional<i32> active_slot;
    std::optional<std::string> name;
    std::optional<Muzzle> muzzle;
    std::optional<Recoil> recoil;
    std::optional<u32> trajectory_generation;
    bool operator==(const Weapon&) const = default;
};
struct Spectators {
    std::optional<Reference> owner;
    std::optional<u32> count;
    bool operator==(const Spectators&) const = default;
};
struct ControllerRotation {
    std::optional<u64> controller_token;
    std::optional<Rotation> rotation;
    bool operator==(const ControllerRotation&) const = default;
};
struct Holding {
    std::optional<bool> accepted_holding;
    std::optional<float> blend;
    bool operator==(const Holding&) const = default;
};
struct Sway {
    std::optional<Rotation> rotation;
    bool operator==(const Sway&) const = default;
};
struct AimBasis {
    std::optional<u64> native_context_token, weapon_token, controller_token;
    std::optional<Reference> owner;
    std::optional<std::string> native_profile;
    std::optional<Vector> direction;
    std::optional<float> zeroing_distance_m, zeroing_correction_degrees, integrated_distance_cm;
    std::optional<u32> integrated_steps, branch, stage;
    std::optional<bool> zeroing_reached_distance, native_zeroing_applied;
    bool operator==(const AimBasis&) const = default;
};
using GroupValue =
    std::variant<Spatial, LivingState, Name, Movement, Skeleton, VehicleState, Seats, VehicleModel, VehicleType,
                 CreatureType, ContainerType, ItemDefinition, ItemClassification, Stack, GroundContents, LocalControl,
                 Camera, CameraTarget, Weapon, Spectators, ControllerRotation, Holding, Sway, AimBasis>;
// 动态子值仍使用自身采样区间；这些是消费时序，不是读取/关闭证明。
struct SourceTiming {
    FactStatus status = FactStatus::NotAttempted;
    u64 capture_sequence = 0;
    SampleInterval source;
    bool reused = false;
    DataCategory source_category = DataCategory::None;
    bool operator==(const SourceTiming&) const noexcept;
};
struct GroupSample {
    FactStatus status = FactStatus::NotAttempted;
    u64 source_capture_sequence = 0;
    SampleInterval source;
    std::optional<GroupValue> value;
    // 字段路径/集合下标沿用业务字段号；每次增量完整刷新此小型表，不能只随变化值发送。
    std::map<std::vector<unsigned>, SourceTiming> sources;
    bool operator==(const GroupSample&) const;
};
struct Entity {
    u64 id = 0;
    EntityKind kind = EntityKind::Unknown;
    std::map<PropertyGroup, GroupSample> groups;
    bool operator==(const Entity&) const = default;
};
struct Pacing {
    double target_hz = 0;
    u64 configuration_revision = 0, plan_revision = 0;
    bool operator==(const Pacing&) const = default;
};
struct Context {
    // token由Reader epoch派生，绝不是进程地址；暂时缺失不补回历史动态值。
    std::optional<u64> world_token, level_token, game_state_token;
    bool native_world_velocity_supported = false;
    std::optional<std::string> map_name;
    std::string coordinate_convention;
    std::optional<double> verified_meters_per_unit;
    std::optional<Origin> origin;
    Pacing pacing;
    std::map<ContextGroup, GroupSample> local_groups;
    std::map<std::vector<unsigned>, SourceTiming> sources;
    void clear() noexcept;
    void swap(Context&) noexcept;
    bool operator==(const Context&) const = default;
};
struct State : PublicationState {
    DataCategory category = DataCategory::None;
    SubscriptionFeatures features = 0;
    Context context;
    std::map<u64, Entity> entities;
    std::map<u64, RemovalReason> removals;
    DataCategory Category() const noexcept override { return category; }
    std::size_t EntityCount() const noexcept override { return entities.size(); }
    bool SameEntity(u64, const PublicationState&) const noexcept override;
    void clear() noexcept;
    void swap(State&) noexcept;
};
bool HasValues(FactStatus) noexcept;
bool GroupAllowed(EntityKind, PropertyGroup) noexcept;
bool LocalGroupAllowed(ContextGroup) noexcept;
bool SameGroupValue(const GroupValue&, const GroupValue&);
bool SameState(const State&, const State&);
bool ValidateState(const State&, std::string& error);
bool SameEpochContext(const Context& confirmed, const Context& current, std::string& error);
bool MergeEpochIdentity(const Context& confirmed, const Context& current, Context& staged, std::string& error);
bool SameEpochContext(const State&, const Context&, std::string& error);
} // namespace monitor::synchronization::consumer
