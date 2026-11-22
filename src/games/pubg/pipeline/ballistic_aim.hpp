#pragma once
#include "control/ballistics.hpp"
#include "games/pubg/pipeline/latency_prediction.hpp"
#include "games/pubg/model/timing_settings.hpp"
namespace mousebox::control
{
enum class BallisticBlock
{
    Disabled,
    Ready,
    CatalogMissing,
    WeaponMissing,
    CameraMissing,
    TargetVelocityMissing,
    SelfVelocityMissing,
    TimingUnavailable,
    UnknownWeapon,
    UnsupportedWeapon,
    NoSolution,
    ZeroingUnreachable,
    InvalidConfiguration
};
struct BallisticDiagnostic
{
    BallisticBlock block = BallisticBlock::Disabled;
    BallisticSolution solution;
    std::string profileId, profileName, revision, sourcePath, sourceBuild, weaponClass;
    double targetAgeMs = 0, cameraAgeMs = 0, uncertaintyMs = 0, initialSpeed = 0;
    double gravityMetersPerSecondSquared = DefaultGravityMetersPerSecondSquared;
    std::optional<ResolvedAimPoint> aim;
    std::optional<Vec3> originWorldMeters;
    std::optional<Identity> weapon;
    enum class TimingPart
    {
        None,
        Target,
        Camera
    } timingPart = TimingPart::None;
    std::optional<PredictionReason> timingReason;
};
BallisticDiagnostic prepareBallisticAim(const Frame &living, const Frame &view, const Target &,
                                        const BallisticCatalog *, unsigned zeroingMeters, std::uint64_t now,
                                        double gravity = DefaultGravityMetersPerSecondSquared,
                                        double livingLocalSkewMs = DefaultLivingLocalSkewMs);
} // namespace mousebox::control
