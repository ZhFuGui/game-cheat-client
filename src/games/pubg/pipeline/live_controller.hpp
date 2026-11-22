#pragma once
#include "games/pubg/pipeline/motion_planner.hpp"
#include "games/pubg/pipeline/target_selector.hpp"
#include "games/pubg/pipeline/ballistic_aim.hpp"
#include "games/pubg/pipeline/native_aim.hpp"
#include "games/pubg/model/weapon_aim.hpp"
#include "control/device_motion.hpp"
#include "control/speed_policy.hpp"
namespace mousebox::control
{
struct ObservedFrames
{
    std::optional<Frame> living, view;
    std::uint64_t livingObservedMs = 0, viewObservedMs = 0;
    bool realtime = false;
};
enum class LiveBlock
{
    Ready,
    NotRealtime,
    DeviceUnavailable,
    LivingMissing,
    ViewMissing,
    PairRejected,
    LivingStale,
    ViewStale,
    CalibrationMissing,
    FovMismatch,
    SelfTeamMissing,
    NoTarget,
    BallisticsUnavailable,
    RotationTimingUnavailable,
    NativeReferenceUnavailable
};
struct LiveDiagnostic
{
    LiveBlock block = LiveBlock::NotRealtime;
    PairVerdict pair;
    SelectionStats selection;
    std::optional<Target> target;
    BallisticDiagnostic ballistics;
    NativeAimEvaluation nativeAim;
};
class LiveController
{
    TargetSelector selector_;
    MotionPlanner planner_;
    LiveDiagnostic diagnostic_;
    AimPart aimPart_ = AimPart::Head;
    WeaponAimPolicy weaponAimPolicy_;
    double maximumDistanceMeters_ = DefaultMaximumAimDistanceMeters;
    TargetPriority priority_ = TargetPriority::NearestDistance;
    std::optional<Identity> heldTarget_;
    struct HoldContext
    {
        Header header;
        std::optional<Identity> self;
        std::optional<std::uint64_t> controller;
    };
    std::optional<HoldContext> heldLiving_, heldView_;
    std::uint64_t heldGeneration_ = 0;
    bool suspended_ = false, terminated_ = false;
    bool heldNative_ = false;
    std::string heldNativeContext_;
    bool ballisticsEnabled_ = false;
    std::shared_ptr<const BallisticCatalog> catalog_;
    unsigned zeroingMeters_ = 100;
    double gravity_ = DefaultGravityMetersPerSecondSquared;
    double livingLocalSkewMs_ = DefaultLivingLocalSkewMs;
    bool changedContext(const ObservedFrames &) const;

  public:
    explicit LiveController(MotionPolicy policy = {20, 600, 6000, 0.05, 0, 0, 5}, AimPart part = AimPart::Head,
                            double maximumDistanceMeters = DefaultMaximumAimDistanceMeters,
                            TargetPriority priority = TargetPriority::NearestDistance, WeaponAimPolicy weaponAim = {});
    void setHumanModel(std::shared_ptr<const HumanMotionModel> model, unsigned dpi, bool variation)
    {
        planner_.setHumanModel(std::move(model), dpi, variation);
    }
    void setBallistics(std::shared_ptr<const BallisticCatalog> catalog, unsigned zeroingMeters,
                       double gravity = DefaultGravityMetersPerSecondSquared,
                       double livingLocalSkewMs = DefaultLivingLocalSkewMs);
    LiveDiagnostic inspect(std::uint64_t now, const ObservedFrames &, const Calibration &, bool device,
                           double sourceSkewMs = 800, std::optional<Identity> required = {});
    const LiveDiagnostic &diagnostic() const noexcept
    {
        return diagnostic_;
    }
    MotionFeedback feedback() const noexcept
    {
        return planner_.feedback();
    }
    bool active() const noexcept
    {
        return planner_.active();
    }
    bool releaseRequired() const noexcept
    {
        return !suspended_ && planner_.releaseRequired();
    }
    MotionEvent stoppedBy() const noexcept
    {
        return suspended_ ? MotionEvent::Suspended : planner_.stoppedBy();
    }
    bool suspended() const noexcept
    {
        return suspended_;
    }
    const std::optional<Identity> &heldTarget() const noexcept
    {
        return heldTarget_;
    }
    void rejectStep(const MotionStep &step) noexcept
    {
        planner_.rejectStep(step);
    }
    MotionStep step(bool down, std::uint64_t generation, std::uint64_t now, const ObservedFrames &, const Calibration &,
                    bool device, double sourceSkewMs = 800);
};
} // namespace mousebox::control
