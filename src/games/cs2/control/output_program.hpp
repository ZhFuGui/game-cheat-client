#pragma once
#include "count_calibration.hpp"
#include "application/ports/output_program.hpp"
#include "control/human_runtime.hpp"
#include "motion/controller.hpp"
#include "feedback/pending_motion.hpp"
#include "static_cover.hpp"
#include "hitbox_aim.hpp"
#include <atomic>
#include <deque>
#include <functional>
#include <mutex>
namespace mousebox::games::cs2
{
struct ControllerState
{
    AimSolution solution;
    CalibrationObservation calibrationInput;
    std::string block = "waiting-source";
    bool down = false, active = false, releaseRequired = true, calibrated = false, model = false;
    bool inputMappingReady = false;
    std::string inputMapping = "native";
    double countScaleX = 0, countScaleY = 0;
    double yawPerCount = 0, pitchPerCount = 0;
    unsigned pending = 0;
    bool calibrating = false;
    unsigned calibrationLeg = 0;
    std::string calibrationStatus;
    std::optional<CountCalibration> calibrationResult;
    std::optional<TargetKey> heldTarget;
    std::string heldTargetName, lastPause, releaseReason, feedbackReference;
    std::optional<std::uint64_t> lastPauseAgeMs;
    std::uint64_t pauseCount = 0;
    bool suspended = false, recoilOnly = false;
    bool variation = false;
    std::string version = "v1", modeId = "v1", comparisonFactor, motionStatus = "v1";
    std::string movementAlgorithm = "smooth";
};
class OutputProgram final : public application::ports::OutputProgram
{
  public:
    using TargetSource = std::function<std::optional<TargetPoint>(const ControlInput &)>;

  private:
    std::function<ControlInput()> observe_;
    TargetSource targetSource_;
    std::atomic<bool> requested_ = false;
    std::atomic<bool> enabled_ = true;
    mutable std::mutex mutex_;
    AimSettings settings_;
    bool coverCheck_ = false;
    mutable StaticCover cover_;
    HitboxAim hitboxAim_;
    control::HumanMotionRuntime human_;
    bool released_ = false, holding_ = false, down_ = false, active_ = false, targetDead_ = false, stopped_ = true;
    std::uint64_t generation_ = 0, last_ = 0, sequence_ = 0;
    std::int64_t sampleStartedNs_ = 0;
    std::optional<TargetKey> held_;
    std::optional<unsigned> heldPointId_;
    std::string heldName_, releaseReason_ = "session-reset";
    std::optional<unsigned> releaseCounter_;
    std::string lastPause_, feedbackReference_ = "unavailable";
    std::uint64_t lastPauseAtMs_ = 0, pauseCount_ = 0, lastTickMs_ = 0;
    std::optional<Angles> awaitFeedback_;
    std::string awaitFeedbackReference_;
    std::uint64_t awaitFeedbackSequence_ = 0;
    std::optional<double> awaitSampleMs_;
    struct SessionIdentity
    {
        std::string reader, clockDomain;
        std::uint64_t epoch;
        std::uint32_t process, controller, pawn, worldGroup;
        bool operator==(const SessionIdentity &) const = default;
    };
    std::optional<SessionIdentity> identity_;
    std::string block_ = "release-then-hold";
    struct RecoilReference
    {
        Angles offset;
        std::optional<std::uint32_t> weapon;
        EyeAngleSource basis;
        std::uint64_t capture;
        std::int64_t sampleStartedNs;
        double simulationTicks, secondsPerTick, uncertaintyMs;
        bool continuousWith(const RecoilReference &) const;
    };
    struct ManualAim
    {
        Angles referenceOffset, commanded;
        Angles direction(Angles effectiveInput) const;
        void record(Angles change);
    };
    std::optional<RecoilReference> recoilReference_;
    std::optional<ManualAim> manualAim_;
    bool rebaseManualAim_ = false;
    motion::Controller motion_;
    std::optional<std::uint64_t> proposalRevision_;
    Angles proposedMovement_;
    std::uint64_t proposalUnresponsiveSince_ = 0;
    double proposalUnconfirmedDegrees_ = 0;
    std::optional<Angles> feedbackAngles_;
    std::uint64_t feedbackSequence_ = 0, unresponsiveSince_ = 0;
    double unconfirmedDegrees_ = 0;
    feedback::PendingMotion pending_;
    CountCalibrationSession calibration_;
    void clearMotion(bool clearPending = true);
    void clearHold();
    void recordPause(const std::string &, std::uint64_t);
    CountMapping mapping(const ControlInput &) const;
    AimSolution solve(const ControlInput &, std::uint64_t, std::optional<HitboxAim::Search> = {}) const;
    bool supportsManualAim() const noexcept;
    Angles manualDirection(const AimSolution &) const;
    void updateMotionReference(const RecoilReference &, bool following);

  public:
    explicit OutputProgram(std::function<ControlInput()>, TargetSource = {},
                           std::shared_ptr<application::ports::VisibilityProvider> = {});
    void setRequested(bool v) noexcept
    {
        requested_ = v;
    }
    bool requested() const noexcept override
    {
        return requested_.load();
    }
    std::optional<control::ActivationBinding> activation() const noexcept override
    {
        return targetSource_ ? std::optional(control::ActivationBinding{control::ActivationSource::Continuous, 0})
                             : std::nullopt;
    }
    std::string lastBlock() const;
    void setEnabled(bool);
    bool enabled() const noexcept
    {
        return enabled_.load();
    }
    void configure(AimSettings);
    void setCoverCheck(bool);
    void setHumanModel(std::shared_ptr<const control::HumanMotionModel>, unsigned, bool);
    application::ports::ProgramStep tick(const application::ports::OutputTick &) override;
    bool current(std::uint64_t, std::uint64_t) const override;
    // Compose a final check using the caller's single immutable observation,
    // rather than reading a second, possibly newer frame midway through it.
    bool current(const ControlInput &, std::uint64_t nowMs) const;
    void reject(const application::ports::ProgramStep &) override;
    void reset() override;
    void startCalibration();
    void cancelCalibration();
    ControllerState inspect(std::uint64_t nowMs, bool solve = true) const;
};
} // namespace mousebox::games::cs2
