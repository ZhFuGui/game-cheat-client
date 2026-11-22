#pragma once
#include "recoil_state.hpp"
#include "accuracy.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace mousebox::games::cs2
{
struct Entity
{
    std::uint32_t id = 0;
    std::string name;
    std::optional<Vec3> position, velocity;
    std::optional<unsigned> team, lifeState;
    std::optional<int> health, maxHealth;
    std::optional<bool> dormant;
    std::optional<Angles> absoluteRotation;
    std::optional<double> absoluteScale;
};
// Semantic roles are independent of each model's native bone indices.
enum class BoneRole : unsigned
{
    Unspecified = 0,
    Pelvis,
    Spine0,
    Spine1,
    Spine2,
    Spine3,
    Neck,
    Head,
    LeftClavicle,
    LeftUpperArm,
    LeftForearm,
    LeftHand,
    RightClavicle,
    RightUpperArm,
    RightForearm,
    RightHand,
    LeftUpperLeg,
    LeftLowerLeg,
    LeftAnkle,
    RightUpperLeg,
    RightLowerLeg,
    RightAnkle
};
enum class SkeletonStatus : unsigned
{
    Unspecified = 0,
    UnknownModel,
    SourceUnavailable,
    VerifiedPartial,
    VerifiedComplete
};
struct Bone
{
    unsigned index = 0;
    Vec3 position;
    std::optional<BoneRole> role;
    std::optional<std::array<double, 4>> worldRotation;
    std::optional<double> worldUniformScale;
};
struct BoneLink
{
    unsigned parent = 0, child = 0;
};
struct Body
{
    Entity entity;
    std::optional<int> armor, shotsFired;
    std::optional<bool> scoped, helmet;
    std::optional<Vec3> networkEyeAngles, inputEyeAngles, viewOffset;
    std::optional<EyeAngleBasis> eyeBasis;
    std::optional<AimPunchState> aimPunch;
    std::optional<std::uint32_t> activeWeapon;
    std::vector<Bone> bones;
    // Absent status means the whole skeleton group was absent in this sample.
    std::optional<SkeletonStatus> skeletonStatus;
    std::optional<std::string> skeletonModel, skeletonProfile;
    std::vector<BoneLink> boneLinks;
    std::optional<bool> nativeHitboxSetAvailable;
};
struct Player
{
    std::uint32_t controller = 0;
    std::optional<std::string> name;
    std::optional<unsigned> team;
    std::optional<bool> local, alive;
    std::optional<std::uint64_t> steamId;
    std::optional<Body> body;
};
struct Weapon
{
    Entity entity;
    std::optional<unsigned> definition;
    std::optional<int> clip, reserve;
    std::optional<double> recoilIndex, accuracyPenalty;
    std::optional<bool> reloading;
    std::optional<WeaponAccuracy> accuracy;
};
struct Local
{
    std::optional<std::uint32_t> controller, pawn, observerTarget;
    std::optional<std::array<double, 16>> matrix;
    std::optional<Vec3> angles, eye;
    std::optional<CameraViewPunch> viewPunch;
    std::optional<MouseMapping> mouseMapping;
    std::optional<bool> crosshairRecoil;
    std::optional<std::uint32_t> cameraRecoilPawn;
    CameraRecoilPath cameraRecoilPath = CameraRecoilPath::Unspecified;
    std::optional<unsigned> fovOverride, observerMode;
};
enum class MapStatus : unsigned
{
    Unspecified = 0,
    Available,
    NotInLevel,
    SourceUnavailable,
    Transition,
    Unsupported
};
// Raw map state belongs to this full snapshot. Names are never inferred from
// positions/files, and non-available states cannot carry a prior map name.
struct MapIdentity
{
    std::string name, resourcePath;
    std::uint64_t generation = 0;
    MapStatus status = MapStatus::Unspecified;
    bool available() const noexcept
    {
        return status == MapStatus::Available && generation && !name.empty() && !resourcePath.empty();
    }
};
struct Snapshot
{
    std::string instance, clockDomain;
    std::uint64_t epoch = 0, sequence = 0, frequency = 0;
    std::uint32_t processId = 0, build = 0;
    std::optional<std::uint64_t> started, readFinished, finished;
    bool directoryVerified = false;
    std::vector<Player> players;
    std::vector<Weapon> weapons;
    std::size_t bombs = 0, projectiles = 0, worldObjects = 0;
    Local local;
    std::optional<MapIdentity> map;
    // One bounded original protobuf Snapshot is retained only in native memory.
    std::string wire;
};
} // namespace mousebox::games::cs2
