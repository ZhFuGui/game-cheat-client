#pragma once
#include "domain/math.hpp"
#include "domain/source_clock.hpp"
#include "games/pubg/model/identity.hpp"
#include "games/pubg/model/bone_role.hpp"
#include <memory>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mousebox::scene
{
struct Snapshot;
}
namespace mousebox::control
{
enum class FrameContract
{
    NativeProof,
    ConsumerV3
};
enum class Category
{
    Living,
    Local
};
struct Header
{
    Category category = Category::Living;
    bool current = false;
    std::string reader, clockDomain, convention;
    std::uint64_t session = 0, epoch = 0, revision = 0, capture = 0, mappingRevision = 0;
    std::int64_t sourceStartNs = 0, sourceFinishNs = 0;
    std::optional<std::uint64_t> world, level;
    bool worldRootUnverified = false, worldCacheUsed = false;
    std::optional<SourceClockEstimate> sourceClock;
    // Consumer-only telemetry identity; never sent to Reader or used by control.
    std::uint64_t consumerConnection = 0;
    FrameContract contract = FrameContract::NativeProof;
    std::string descriptorSha256;
    std::optional<double> consumerReceivedAtMs;
};
struct Bone
{
    unsigned index = 0;
    std::optional<int> parent;
    Vec3 position, origin;
    std::string name;
    std::int64_t sourceStartNs = 0, sourceFinishNs = 0;
    std::optional<int> poseIndex;
    // Consumer roles are semantic topology, never native index/pose substitutes.
    std::optional<BoneRole> role, parentRole;
};
struct NativeMovement
{
    Vec3 componentVelocity; // Native MovementComponent.Velocity, NOT total body-world velocity.
    Identity component;
    std::optional<std::uint64_t> rawComponent;
    std::int64_t sourceStartNs = 0, sourceFinishNs = 0;
};
enum class BodyVelocitySource
{
    ReaderTotalWorld,
    NativeComponentAssumingZeroParent
};
struct BodyWorldVelocity
{
    Identity owner; // Must match the complete selected character lifecycle.
    Vec3 value;     // World-axis units/s; source states whether parent motion is proven or assumed zero.
    std::int64_t sourceStartNs = 0, sourceFinishNs = 0;
    BodyVelocitySource source = BodyVelocitySource::ReaderTotalWorld;
    unsigned nativeSource = 0; // Reader's actual branch code; not inferred from velocity magnitude.
};
struct MuzzleSample
{
    Vec3 position, origin;
    std::int64_t sourceStartNs = 0, sourceFinishNs = 0;
};
// Raw native remaining input targets, NOT degrees or already applied recoil.
// The source interval covers separate target-pair / recovery reads, not an atomic rotation.
struct WeaponRecoilTargets
{
    std::optional<double> pitch, yaw, recovery;
    std::int64_t sourceStartNs = 0, sourceFinishNs = 0;
};
// Native uint32 trajectory-record generation counter, not firing state or shots.
struct WeaponTrajectoryGeneration
{
    std::uint32_t counter = 0;
    std::int64_t sourceStartNs = 0, sourceFinishNs = 0;
};
struct ControllerRotationSample
{
    Identity controller; // Independent controller lifecycle; no body/camera substitution.
    Vec3 rotation;       // Raw GetControlRotation degrees, NOT proven firing direction.
    std::int64_t sourceStartNs = 0, sourceFinishNs = 0;
};
// Independent Local observations. Optional groups retain failed terminal states;
// values never borrow camera/weapon timestamps or become mouse input implicitly.
struct NativeFieldSource
{
    std::int64_t startNs = 0, finishNs = 0;
    std::uint64_t capture = 0;
    unsigned origin = 0; // Consumer business source=0; Debug direct=1 / actual-input calculation=2.
};
struct NativeObservation
{
    unsigned status = 4; // Original Reader FactStatus, not its wire enum.
    bool current = false;
    std::uint64_t capture = 0;
    std::int64_t verifiedStartNs = 0, verifiedFinishNs = 0;
    std::map<unsigned, NativeFieldSource> fields;
    FrameContract contract = FrameContract::NativeProof;
    std::int64_t sourceStartNs = 0, sourceFinishNs = 0; // Consumer has no Debug verification interval.
};
struct NativeHoldingSample
{
    NativeObservation observation;
    std::optional<Identity> character, component;
    std::optional<bool> accepted;
    std::optional<double> blend;
};
struct NativeSwaySample
{
    NativeObservation observation;
    std::optional<Identity> character, weapon;
    std::optional<Vec3> rotation; // Native mixed FRotator degrees, not pure breath.
};
struct CurrentItemSample
{
    NativeObservation observation;
    std::optional<Identity> character;
    std::optional<bool> hasCurrentItem; // False is verified empty; missing is unknown.
};
struct NativeAimBasisSample
{
    NativeObservation observation;
    std::optional<Identity> character, weapon, mesh, gunData, trajectoryConfig;
    std::optional<Vec3> direction; // World forward only; contains no roll/input frame.
    std::optional<double> zeroingDistanceMeters, zeroingCorrectionDegrees;
    std::optional<std::uint32_t> integratedSteps;
    std::optional<bool> reachedDistance;
    std::optional<double> integratedDistanceCm;
    unsigned branch = 0, stage = 0;
    bool nativeZeroingApplied = false;
    std::string nativeProfile;
    std::optional<std::uint64_t> nativeContextToken, controllerToken;
};
struct ActiveWeapon
{
    Identity owner, instance;
    int slot = -1;
    std::string actorClass, itemId;
    std::optional<MuzzleSample> muzzle;
    std::optional<WeaponRecoilTargets> recoilTargets;               // Telemetry only; never changes input mapping.
    std::optional<WeaponTrajectoryGeneration> trajectoryGeneration; // Optional zero is a real counter.
};
struct Character
{
    Identity identity;
    std::optional<int> team;
    std::optional<Vec3> head, headOrigin;
    std::string name;
    std::vector<Bone> bones;
    std::optional<NativeMovement> nativeMovement;
    // Live projection uses Reader's independently observed total-world value.
    // Missing full velocity is not a stationary character; legacy estimates
    // remain distinguishable for old offline control fixtures.
    std::optional<BodyWorldVelocity> bodyWorldVelocity;
    bool nativeMovementExpected = false;
    // Presentation facts only; ESP must not substitute head/bones for Actor Spatial.
    std::optional<Vec3> position, positionOrigin;
    std::optional<bool> isBot;
    struct SkeletonBinding
    {
        Identity owner;
        std::uint64_t layoutToken = 0, poseToken = 0;
    };
    std::optional<SkeletonBinding> skeletonBinding;
};
struct Frame
{
    std::shared_ptr<const scene::Snapshot> scene; // Derived presentation boundary; never qualifies control.
    Header header;
    std::optional<Identity> self;
    std::optional<int> selfTeam;
    std::optional<std::uint64_t> controller;
    std::vector<Character> characters;
    std::optional<Vec3> camera, cameraOrigin, rotation; // pitch, yaw, roll in x/y/z
    std::optional<double> fov;
    std::optional<double> verifiedMetersPerUnit;
    std::string verifiedMapName;
    std::optional<std::uint64_t> presentationWorld;
    bool cameraSourceAvailable = false;
    std::optional<std::uint64_t> presentationGameState;
    std::optional<ActiveWeapon> activeWeapon; // Current local body, never ViewTarget or remembered weapon.
    std::optional<CurrentItemSample> currentItem;
    std::optional<unsigned> spectatedCount;                     // Local body only; present zero differs from missing.
    std::optional<ControllerRotationSample> controllerRotation; // Independent input-axis source, not camera feedback.
    std::optional<NativeHoldingSample> nativeHolding;
    std::optional<NativeSwaySample> nativeSway;
    std::optional<NativeAimBasisSample> nativeAimBasis;
    std::int64_t cameraSourceStartNs = 0, cameraSourceFinishNs = 0;
    std::int64_t rotationSourceStartNs = 0, rotationSourceFinishNs = 0;
};
struct Calibration
{
    bool measured = false;
    std::string profile;
    double yawDegreesPerCount = 0, pitchDegreesPerCount = 0, fov = 0;
    bool formulaEstimated = false; // An input-response estimate is never persisted as a measurement.
};
struct AngularError
{
    double horizontal = 0, vertical = 0, cone = 0, distanceUnits = 0;
};
struct ResolvedAimPoint
{
    Vec3 position, origin;            // Virtual sight point already aligned to decision time.
    std::optional<Vec3> nextPosition; // Same virtual camera origin, decision time +20ms.
    Identity weapon;
    std::string modelRevision;
    std::optional<Vec3> launchDirection,
        nextLaunchDirection; // Before equivalent-zero removal; native basis already contains zeroing.
};
// A qualified native forward and the independently observed Controller yaw.
// Input mapping is a declared local rigid-pose model, not a measured native Jacobian.
struct NativeAimReference
{
    Vec3 direction;
    double controllerYawDegrees = 0;
    std::int64_t sourceStartNs = 0, sourceFinishNs = 0;
    std::string context;
};
struct Target
{
    Identity identity;
    AngularError error;
    std::optional<Vec3> position, origin; // Selected current Reader bone geometry for local command-effect estimation.
    std::int64_t sourceFinishNs = 0;
    std::int64_t sourceStartNs = 0;
    std::optional<BodyWorldVelocity> bodyWorldVelocity;
    std::optional<SourceClockEstimate> sourceClock;
    bool nativeMovementExpected = false;
    std::optional<ResolvedAimPoint> resolvedAim;
    std::optional<NativeAimReference> nativeAim;
};
std::optional<AngularError> angleTo(const Frame &view, Vec3 position, Vec3 origin);
bool validCalibration(const Calibration &) noexcept;
} // namespace mousebox::control
