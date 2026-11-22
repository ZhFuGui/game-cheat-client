#pragma once
#include "games/pubg/model/control_frame.hpp"
#include "control/human_runtime.hpp"
#include "games/pubg/pipeline/latency_prediction.hpp"
#include "control/motion_policy.hpp"
#include "control/motion_step.hpp"
#include "control/movement/trajectory_smooth.hpp"
#include <deque>
namespace mousebox::control
{
// Declared control-model assumptions, not measurements of the box/game pipeline.
struct AngularFeedbackTuning
{
    static constexpr double responseMs = 20, windowMs = 160, minimumWindowMs = 80;
    static constexpr double maximumRateDegreesPerSecond = 45, maximumLeadDegrees = 1.5;
    static constexpr double minimumRateDegreesPerSecond = 0.75;
    static constexpr double trackingGain = 20;
};
struct MotionFeedback
{
    SampleTimeEstimate timing;
    bool tracking = false, residualReady = false;
    double yawRate = 0, pitchRate = 0, yawLead = 0, pitchLead = 0;
    unsigned pendingCommands = 0;
    bool nativeReference = false;
};
class MotionPlanner
{
    MotionPolicy policy_;
    bool released_ = false, active_ = false;
    std::uint64_t generation_ = 0, deadline_ = 0, last_ = 0;
    Identity target_;
    std::optional<std::pair<Identity, std::string>> resolvedContext_;
    movement::TrajectorySmooth trajectory_;
    Vec3 previousPoint_, pointVelocity_;
    std::int64_t pointSourceNs_ = 0;
    std::uint64_t pointObservedMs_ = 0;
    unsigned spent_ = 0;
    MotionEvent stoppedBy_ = MotionEvent::None;
    HumanMotionRuntime human_;
    bool settled_ = false;
    bool feedbackSeen_ = false;
    std::int64_t viewStartNs_ = 0, viewFinishNs_ = 0;
    Vec3 observedRotation_;
    double pendingYaw_ = 0, pendingPitch_ = 0;
    bool nativeReference_ = false;
    double pendingInputYaw_ = 0;
    struct PendingMotion
    {
        std::uint64_t at = 0;
        double yaw = 0, pitch = 0, inputYaw = 0;
    };
    std::deque<PendingMotion> pending_;
    bool responseAligned_ = false, creditedLastStep_ = false;
    std::int64_t responseSourceNs_ = 0;
    double responseLocalMs_ = 0;
    std::uint64_t unresponsiveSince_ = 0;
    bool sourceTimed_ = false, tracking_ = false;
    double residualTimeMs_ = 0, disturbanceYawRate_ = 0, disturbancePitchRate_ = 0;
    double disturbanceYawLead_ = 0, disturbancePitchLead_ = 0;
    bool residualReady_ = false;
    SampleTimeEstimate feedbackTiming_;
    struct AngularResidual
    {
        double at = 0, span = 0, yaw = 0, pitch = 0;
    };
    std::deque<AngularResidual> angularResiduals_;
    bool reconcile(const Frame &, const Calibration &, std::uint64_t, const NativeAimReference *native = nullptr);
    void reset() noexcept;

  public:
    explicit MotionPlanner(MotionPolicy = {});
    void setHumanModel(std::shared_ptr<const HumanMotionModel> model, unsigned dpi, bool variation)
    {
        reset();
        human_.configure(std::move(model), dpi, variation);
        released_ = false;
    }
    bool active() const noexcept
    {
        return active_;
    }
    bool releaseRequired() const noexcept
    {
        return !released_ && !active_;
    }
    MotionEvent stoppedBy() const noexcept
    {
        return stoppedBy_;
    }
    MotionFeedback feedback() const noexcept
    {
        return {
            feedbackTiming_,       tracking_,           residualReady_,        disturbanceYawRate_,
            disturbancePitchRate_, disturbanceYawLead_, disturbancePitchLead_, static_cast<unsigned>(pending_.size()),
            nativeReference_};
    }
    void rejectStep(const MotionStep &) noexcept;
    MotionStep cancel(MotionEvent) noexcept;
    MotionStep step(bool down, std::uint64_t generation, std::uint64_t nowMs, std::optional<Target> target,
                    const Calibration &, bool eligible, const Frame *view = nullptr);
};
} // namespace mousebox::control
