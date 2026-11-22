#include "output_program.hpp"
#include "modes.hpp"
#include "pipeline/reference.hpp"
#include "pipeline/angles.hpp"
#include <algorithm>
#include <cmath>
namespace mousebox::games::cs2
{
namespace
{
double angleChange(Angles a, Angles b)
{
    return std::hypot(std::remainder(a.y - b.y, 360.0), std::remainder(a.x - b.x, 360.0));
}
bool hasRecoil(Angles offset) noexcept
{
    return offset.x != 0 || offset.y != 0;
}
} // namespace
OutputProgram::OutputProgram(std::function<ControlInput()> observe, TargetSource targetSource,
                             std::shared_ptr<application::ports::VisibilityProvider> geometry)
    : observe_(std::move(observe)), targetSource_(std::move(targetSource)), cover_(std::move(geometry))
{
}
AimSolution OutputProgram::solve(const ControlInput &in, std::uint64_t now, std::optional<HitboxAim::Search> search) const
{
    if (!targetSource_)
    {
        std::optional<StaticCover::Access> access;
        const auto scene = [&]() -> const StaticCover::Access & {
            if (!access)
                access = cover_.acquire(in);
            return *access;
        };
        AimPointCheck check;
        if (coverCheck_)
            check = [&](Vec3 origin, Vec3 point) { return scene().check(origin, point); };
        AimPointResolver resolvePoint;
        if (settings_.hitboxAim)
            resolvePoint = [&](const Body &body, TargetKey key, unsigned part, const AimPointProbe &probe) {
                if (probe)
                {
                    auto resolution = hitboxAim_.visiblePoint(*in.frame, body, key, part, *settings_.hitboxAim, now,
                                                              search, !down_, probe);
                    if (!search && resolution.block == "cover-searching" && !scene().scene)
                        resolution.block = scene().block;
                    return resolution;
                }
                const auto point = hitboxAim_.point(*in.frame, body, key, part, *settings_.hitboxAim);
                return AimPointResolution{point, point ? "" : "aim-hitbox-unavailable"};
            };
        return solveAim(in, settings_, now, held_, check, resolvePoint);
    }
    const auto point = targetSource_(in);
    if (point)
        return solveAimAt(in, settings_, now, *point);
    AimSolution out;
    out.block = "no-target";
    return out;
}
bool OutputProgram::supportsManualAim() const noexcept
{
    return !targetSource_ && settings_.recoil;
}
bool OutputProgram::RecoilReference::continuousWith(const RecoilReference &before) const
{
    if (weapon != before.weapon || basis != before.basis || secondsPerTick != before.secondsPerTick)
        return false;
    if (capture == before.capture)
        return true;
    const auto samplingMs = static_cast<double>(sampleStartedNs - before.sampleStartedNs) / 1000000;
    const auto simulationMs = (simulationTicks - before.simulationTicks) * secondsPerTick * 1000;
    const auto tolerance = secondsPerTick * 1000 + uncertaintyMs + before.uncertaintyMs;
    return capture > before.capture && samplingMs >= 0 && simulationMs >= 0 &&
           std::abs(samplingMs - simulationMs) <= tolerance;
}
Angles OutputProgram::ManualAim::direction(Angles effectiveInput) const
{
    return {effectiveInput.x + referenceOffset.x - commanded.x,
            std::remainder(effectiveInput.y + referenceOffset.y - commanded.y, 360.0), effectiveInput.z};
}
void OutputProgram::ManualAim::record(Angles change)
{
    commanded.x += change.x;
    commanded.y += change.y;
}
Angles OutputProgram::manualDirection(const AimSolution &reference) const
{
    const auto &direction = *reference.direction;
    const auto &recoil = *reference.recoilOffset;
    const auto outstanding = pending_.residual({});
    return manualAim_->direction({direction.x - recoil.x - outstanding.x,
                                  direction.y - recoil.y - outstanding.y, direction.z - recoil.z});
}
void OutputProgram::updateMotionReference(const RecoilReference &reference, bool following)
{
    const bool continuous = recoilReference_ && reference.continuousWith(*recoilReference_);
    if (following && manualAim_)
    {
        manualAim_.reset();
        motion_.reset();
        human_.reset();
    }
    else if (!following && (!manualAim_ || !continuous))
    {
        // A new hold uses the uncompensated input direction. On loss of a
        // followed target, keep the shot direction already established there.
        // After a source or weapon discontinuity, start at the current shot
        // direction so no missed correction is replayed.
        const auto offset = continuous ? recoilReference_->offset
                            : rebaseManualAim_ || recoilReference_ ? reference.offset
                                                                  : Angles{};
        manualAim_ = ManualAim{offset, {}};
        motion_.reset();
        human_.reset();
    }
    recoilReference_ = reference;
    rebaseManualAim_ = false;
}
std::string OutputProgram::lastBlock() const
{
    std::lock_guard lock(mutex_);
    return block_;
}
void OutputProgram::clearMotion(bool clearPending)
{
    active_ = false;
    rebaseManualAim_ |= recoilReference_.has_value();
    recoilReference_.reset();
    manualAim_.reset();
    proposalRevision_.reset();
    motion_.reset();
    if (clearPending)
        pending_.clear();
    last_ = 0;
    feedbackAngles_.reset();
    feedbackSequence_ = unresponsiveSince_ = 0;
    unconfirmedDegrees_ = 0;
    human_.reset();
}
void OutputProgram::clearHold()
{
    holding_ = false;
    rebaseManualAim_ = false;
    held_.reset();
    heldPointId_.reset();
    hitboxAim_.reset();
    heldName_.clear();
    targetDead_ = false;
    identity_.reset();
    sequence_ = 0;
    sampleStartedNs_ = 0;
    awaitFeedback_.reset();
    awaitSampleMs_.reset();
}
void OutputProgram::recordPause(const std::string &reason, std::uint64_t now)
{
    if ((holding_ || active_) && (active_ || block_ != reason))
    {
        lastPause_ = reason;
        lastPauseAtMs_ = now;
        ++pauseCount_;
    }
}
void OutputProgram::reset()
{
    std::lock_guard lock(mutex_);
    cover_.reset();
    clearMotion();
    clearHold();
    released_ = down_ = false;
    releaseCounter_.reset();
    releaseReason_ = "session-reset";
    block_ = "release-then-hold";
    stopped_ = true;
    if (calibration_.active())
        calibration_.cancel();
}
void OutputProgram::configure(AimSettings s)
{
    std::lock_guard lock(mutex_);
    if (s == settings_)
        return;
    settings_ = std::move(s);
    clearMotion();
    clearHold();
    released_ = false;
    releaseCounter_.reset();
    releaseReason_ = "settings-changed";
    if (calibration_.active())
        calibration_.cancel("settings-changed");
}
void OutputProgram::setEnabled(bool enabled)
{
    std::lock_guard lock(mutex_);
    if (enabled_.exchange(enabled) == enabled)
        return;
    clearMotion();
    clearHold();
    released_ = down_ = false;
    releaseCounter_.reset();
    releaseReason_ = "settings-changed";
    block_ = enabled ? "release-then-hold" : "disabled";
    if (calibration_.active())
        calibration_.cancel("settings-changed");
}
void OutputProgram::setCoverCheck(bool enabled)
{
    std::lock_guard lock(mutex_);
    if (coverCheck_ == enabled)
        return;
    coverCheck_ = enabled;
    cover_.reset();
    clearMotion();
    clearHold();
    released_ = false;
    releaseCounter_.reset();
    releaseReason_ = "settings-changed";
    if (calibration_.active())
        calibration_.cancel("settings-changed");
}
void OutputProgram::setHumanModel(std::shared_ptr<const control::HumanMotionModel> m, unsigned dpi, bool variation)
{
    std::lock_guard lock(mutex_);
    human_.configure(std::move(m), dpi, variation);
    clearMotion();
    clearHold();
    released_ = false;
    releaseCounter_.reset();
    releaseReason_ = "model-changed";
}
CountMapping OutputProgram::mapping(const ControlInput &in) const
{
    return in.current && in.frame && in.frame->local.mouseMapping && nativeMouse(*in.frame->local.mouseMapping)
               ? resolveCountMapping(settings_, in.frame->build)
               : CountMapping{};
}
void OutputProgram::startCalibration()
{
    std::lock_guard lock(mutex_);
    clearMotion();
    clearHold();
    released_ = false;
    releaseCounter_.reset();
    releaseReason_ = "calibration-started";
    calibration_.start();
}
void OutputProgram::cancelCalibration()
{
    reset();
    std::lock_guard lock(mutex_);
    calibration_.cancel();
}
application::ports::ProgramStep OutputProgram::tick(const application::ports::OutputTick &requestedTick)
{
    const auto in = observe_();
    auto t = requestedTick;
    t.nowMs = std::max(t.nowMs, in.observedAtMs);
    std::lock_guard lock(mutex_);
    lastTickMs_ = t.nowMs;
    proposalRevision_.reset();
    const auto modePolicy = pipelinePolicy({settings_.version, settings_.comparison});
    application::ports::ProgramStep out;
    out.sourceRevision = in.revision;
    const auto pause = [&](std::string reason, bool release = false) {
        recordPause(reason, t.nowMs);
        clearMotion(release);
        block_ = std::move(reason);
        if (release)
        {
            released_ = false;
            clearHold();
            releaseReason_ = block_;
        }
        out.motion.stop = !stopped_;
        stopped_ = true;
        out.motion.event = control::MotionEvent::Suspended;
        out.releaseRequired = !released_ && !holding_;
        return out;
    };
    if (generation_ && generation_ != t.generation)
    {
        generation_ = t.generation;
        clearMotion();
        clearHold();
        released_ = false;
        releaseCounter_.reset();
        releaseReason_ = "context-changed";
        if (calibration_.active())
            calibration_.cancel("context-changed");
    }
    generation_ = t.generation;
    // An explicit target source is continuous activation, not a fabricated
    // physical press. Real side-key release counters belong to manual aiming.
    down_ = t.buttonFresh && (targetSource_ || t.down);
    const bool releasedSinceLast =
        t.buttonFresh && t.releaseCounter && releaseCounter_ && *t.releaseCounter != *releaseCounter_;
    if (t.buttonFresh && t.releaseCounter)
        releaseCounter_ = t.releaseCounter;
    if (calibration_.active())
    {
        auto calibrationTick = t;
        if (releasedSinceLast)
            calibrationTick.down = false;
        out.motion = calibration_.step(in, calibrationTick, settings_);
        if (out.motion.dx || out.motion.dy)
            stopped_ = false;
        if (out.motion.stop)
            stopped_ = true;
        block_ = "calibrating";
        out.active = calibration_.active();
        return out;
    }
    if (!enabled_)
        return pause("disabled", true);
    if (!t.buttonFresh)
        return pause("button-stale", !releaseCounter_);
    if (!targetSource_ && (!t.down || releasedSinceLast))
    {
        clearMotion();
        clearHold();
        released_ = true;
        releaseReason_.clear();
        block_ = "waiting-side-key";
        out.motion.stop = !stopped_;
        stopped_ = true;
        // A real release/repress can be hidden inside a feedback gap. End the
        // old hold first; a still-down fresh level starts its new intent next tick.
        return out;
    }
    if (targetSource_ && !requested())
        return pause("waiting-source", true);
    if (!targetSource_ && !released_ && !holding_)
        return pause(releaseReason_ == "context-changed" || releaseReason_ == "device-unavailable"
                         ? releaseReason_
                         : "release-then-hold");
    const auto countMapping = mapping(in);
    if (!countMapping.ready)
        return pause("calibration-required");
    if (!t.limits.valid())
        return pause("device-unavailable", true);
    std::optional<HitboxAim::Search> search;
    if (!targetSource_ && settings_.hitboxAim)
    {
        if (coverCheck_)
            search = hitboxAim_.startSearch(held_.has_value());
        else
            hitboxAim_.advance(t.nowMs, settings_.hitboxAim->intervalMs);
    }
    auto solution = solve(in, t.nowMs, search);
    if (targetSource_ && solution.ready() && holding_ &&
        (solution.target != held_ || solution.pointId != heldPointId_))
    {
        clearMotion();
        clearHold();
    }
    if (solution.block == "held-target-dead")
        targetDead_ = true;
    const bool following = solution.ready() && !targetDead_;
    std::string targetBlock;
    if (!following)
    {
        targetBlock = targetDead_ ? "held-target-dead" : std::move(solution.block);
        if (!supportsManualAim())
            return pause(std::move(targetBlock));
        solution = solveManualAimReference(in, settings_, t.nowMs);
        if (!solution.ready())
            return pause(solution.block);
    }

    const auto &f = *in.frame;
    const auto &self = *localBody(f);
    const auto &times = *self.aimPunch->times;
    const SessionIdentity id{f.instance,          f.clockDomain, f.epoch,          f.processId,
                             *f.local.controller, *f.local.pawn, *times.worldGroup};
    // Selector and simulation time can change with the prediction route.
    // The source identity, capture order and source-read timestamp cannot.
    if (identity_ && *identity_ != id)
        return pause("context-changed", true);
    identity_ = id;
    if (sequence_ && (f.sequence < sequence_ || (f.sequence > sequence_ && in.sample->sourceStartNs < sampleStartedNs_)))
        return pause("clock-regressed");
    sampleStartedNs_ = in.sample->sourceStartNs;
    sequence_ = f.sequence;

    const auto basis = self.eyeBasis ? self.eyeBasis->source : EyeAngleSource::Unspecified;
    const auto simulationTicks = *times.simulation->tick + *times.simulation->fraction;
    const RecoilReference recoil{*solution.recoilOffset, self.activeWeapon, basis, f.sequence,
                                 sampleStartedNs_, simulationTicks, *times.secondsPerTick, solution.timing.uncertaintyMs};
    if (!following && !manualAim_ && !hasRecoil(recoil.offset) &&
        (!recoilReference_ || !hasRecoil(recoilReference_->offset)))
    {
        // An idle manual direction consumes no motion cadence or velocity
        // from the target controller. Keep a baseline for the first recoil.
        const auto idle = pause(std::move(targetBlock));
        recoilReference_ = recoil;
        rebaseManualAim_ = false;
        return idle;
    }
    if (following)
    {
        if (!held_)
        {
            held_ = solution.target;
            heldName_ = solution.targetName;
        }
        if (search && solution.pointSeed && solution.pointId)
            hitboxAim_.keepVisible(*solution.target, *solution.pointId, *solution.pointSeed, t.nowMs);
        else if (settings_.hitboxAim && solution.pointId != heldPointId_)
            hitboxAim_.keepUntilNextInterval(t.nowMs);
        heldPointId_ = solution.pointId;
    }
    if (!holding_)
    {
        holding_ = true;
        released_ = false;
        out.motion.begin = true;
    }
    const auto policy = control::speedPolicy(settings_.speed, t.limits);
    if (last_ && t.nowMs >= last_ && t.nowMs - last_ < policy.intervalMs)
    {
        out.active = active_;
        return out;
    }
    if (last_ && (t.nowMs < last_ || t.nowMs - last_ > 100))
        return pause("scheduler-stall");
    const double dt = last_ ? static_cast<double>(t.nowMs - last_) / 1000 : policy.intervalMs / 1000.0;
    last_ = t.nowMs;
    // Remove only commands that the sampled input is expected to contain. This
    // is an explicit response model, not a device/game acknowledgement.
    pending_.retire(solution.timing.localSampleMs, settings_.responseMs);
    const auto &m = *in.frame->local.mouseMapping;
    const double yaw = *m.yawDegreesPerDelta * countMapping.x;
    const double pitch = *m.pitchDegreesPerDelta * countMapping.y;
    const auto [angles, reference] = pipeline::responseInput(*in.frame, following ? modePolicy : PipelinePolicy{});
    feedbackReference_ = reference;
    const double responseThreshold = std::max(.005, std::min(std::abs(yaw), std::abs(pitch)) * .25);
    if (awaitSampleMs_)
    {
        if (solution.timing.localSampleMs < *awaitSampleMs_)
            return pause("feedback-delayed");
        awaitSampleMs_.reset();
    }
    if (awaitFeedback_)
    {
        if (feedbackReference_ != awaitFeedbackReference_ || in.frame->sequence <= awaitFeedbackSequence_ ||
            angleChange(angles, *awaitFeedback_) <= responseThreshold)
            return pause("feedback-timeout");
        awaitFeedback_.reset();
    }
    if (pending_.expired(t.nowMs))
    {
        awaitSampleMs_ = pending_.expectedAfter(settings_.responseMs);
        return pause("feedback-delayed");
    }
    if (feedbackSequence_ != in.frame->sequence)
    {
        if (feedbackAngles_ && angleChange(angles, *feedbackAngles_) > responseThreshold)
        {
            unresponsiveSince_ = 0;
            unconfirmedDegrees_ = 0;
        }
        feedbackAngles_ = angles;
        feedbackSequence_ = in.frame->sequence;
    }
    if (unresponsiveSince_ && t.nowMs - unresponsiveSince_ > 500 && unconfirmedDegrees_ > .25)
    {
        awaitFeedback_ = angles;
        awaitFeedbackReference_ = feedbackReference_;
        awaitFeedbackSequence_ = in.frame->sequence;
        return pause("feedback-timeout");
    }
    if (supportsManualAim())
        updateMotionReference(recoil, following);

    motion::Input motionInput;
    if (following)
        motionInput.error = pending_.residual(*solution.error, modePolicy.subtractPending);
    else
    {
        // E + pending is where input is expected to be at execution. Remove
        // our own recoil commands from that angle to recover the manual intent.
        // The same pending motion appears in the predicted shot direction.
        const auto offset = pending_.residual({});
        const auto &shot = *solution.direction;
        const Angles effectiveShot{shot.x - offset.x, shot.y - offset.y, shot.z};
        motionInput.error = pipeline::angularError(manualDirection(solution), effectiveShot);
    }
    motionInput.recoil = *solution.recoilOffset;
    motionInput.yawPerCount = yaw;
    motionInput.pitchPerCount = pitch;
    motionInput.dt = dt;
    motionInput.policy = policy;
    if (following)
        motionInput.policy.trajectory = settings_.trajectory;
    motionInput.now = t.nowMs;
    motionInput.capture = in.frame->sequence;
    motionInput.sourceStartNs = in.sample->sourceStartNs;
    motionInput.simulationTicks = simulationTicks;
    motionInput.secondsPerTick = *times.secondsPerTick;
    motionInput.uncertaintyMs = solution.timing.uncertaintyMs;
    motionInput.basisSource = settings_.version == AimSettings::Version::V4 ? EyeAngleSource::Unspecified : basis;
    motionInput.recoilEnabled = settings_.recoil;
    const auto algorithm = following ? resolvedMovementAlgorithm(settings_) : control::movement::Algorithm::Direct;
    const auto proposed = motion_.step(algorithm, motionInput, human_, following ? modePolicy.motion : MotionFactors{});
    out.motion.dx = proposed.counts.x;
    out.motion.dy = proposed.counts.y;
    proposalUnresponsiveSince_ = unresponsiveSince_;
    proposalUnconfirmedDegrees_ = unconfirmedDegrees_;
    if (out.motion.dx || out.motion.dy)
    {
        proposedMovement_ = {out.motion.dy * pitch, out.motion.dx * yaw, 0};
        proposalRevision_ = in.revision;
        if (manualAim_)
            manualAim_->record(proposedMovement_);
        pending_.add(t.nowMs, proposedMovement_.y, proposedMovement_.x);
        if (!unresponsiveSince_)
            unresponsiveSince_ = t.nowMs;
        unconfirmedDegrees_ += std::hypot(proposedMovement_.y, proposedMovement_.x);
        stopped_ = false;
    }
    active_ = out.active = true;
    block_ = following ? "tracking" : "recoil-only";
    out.motion.event = out.motion.begin ? control::MotionEvent::Begin : control::MotionEvent::None;
    return out;
}
bool OutputProgram::current(std::uint64_t revision, std::uint64_t now) const
{
    const auto in = observe_();
    return in.revision == revision && current(in, now);
}
bool OutputProgram::current(const ControlInput &in, std::uint64_t now) const
{
    now = std::max(now, in.observedAtMs);
    std::lock_guard lock(mutex_);
    if (!in.current)
        return false;
    if (calibration_.active())
        return observeCalibration(in, settings_, now).ready();
    if (!enabled_ || !holding_ || awaitFeedback_ || awaitSampleMs_ || !mapping(in).ready)
        return false;
    const auto solution = solve(in, now);
    if (manualAim_)
        return down_ && supportsManualAim() && (targetDead_ || !solution.ready()) &&
               solveManualAimReference(in, settings_, now).ready();
    if (!solution.ready() || targetDead_ || (targetSource_ && !requested()))
        return false;
    return (!targetSource_ && !settings_.hitboxAim) ||
           (solution.target == held_ && solution.pointId == heldPointId_);
}
void OutputProgram::reject(const application::ports::ProgramStep &step)
{
    std::lock_guard lock(mutex_);
    if (!calibration_.active() && proposalRevision_ != step.sourceRevision)
        return;
    proposalRevision_.reset();
    recordPause("output-rejected", lastTickMs_);
    pending_.rejectLast();
    if (manualAim_)
        manualAim_->record({-proposedMovement_.x, -proposedMovement_.y, 0});
    if (motion_.retryAfterReject() && !calibration_.active())
    {
        motion_.reject();
        active_ = false;
        stopped_ = true;
        unresponsiveSince_ = proposalUnresponsiveSince_;
        unconfirmedDegrees_ = proposalUnconfirmedDegrees_;
    }
    else
        clearMotion(false);
    if (calibration_.active())
        calibration_.cancel("output-rejected");
    block_ = "output-rejected";
}
ControllerState OutputProgram::inspect(std::uint64_t now, bool solve) const
{
    const auto in = observe_();
    now = std::max(now, in.observedAtMs);
    std::lock_guard lock(mutex_);
    ControllerState r;
    bool manualReady = false;
    if (solve && enabled_)
    {
        r.solution = this->solve(in, now);
        if (supportsManualAim() && (targetDead_ || !r.solution.ready()))
        {
            r.solution = solveManualAimReference(in, settings_, now);
            manualReady = r.solution.ready();
            if (manualReady && manualAim_)
            {
                r.solution.desired = manualDirection(r.solution);
                const auto offset = pending_.residual({});
                const auto &shot = *r.solution.direction;
                r.solution.error = pipeline::angularError(*r.solution.desired, {shot.x - offset.x, shot.y - offset.y, shot.z});
            }
        }
    }
    else
        r.solution.block = "disabled";
    r.calibrationInput = observeCalibration(in, settings_, now);
    const auto countMapping = mapping(in);
    r.calibrated = countMapping.measured;
    r.inputMappingReady = countMapping.ready;
    r.inputMapping = settings_.inputMapping == AimSettings::InputMapping::NativeFormula ? "native" : "measured";
    r.countScaleX = countMapping.x;
    r.countScaleY = countMapping.y;
    r.block = calibration_.active()            ? "calibrating"
              : !supportsManualAim() && targetDead_ ? "held-target-dead"
              : !r.solution.ready()                ? r.solution.block
              : !r.inputMappingReady               ? "calibration-required"
              : manualReady && block_ == "waiting-side-key" ? "recoil-ready"
                                                            : block_;
    r.active = active_ && r.solution.ready() && r.inputMappingReady;
    r.recoilOnly = r.active && manualReady && manualAim_.has_value();
    r.down = down_;
    r.releaseRequired = !released_ && !holding_;
    r.model = human_.enabled();
    r.variation = human_.variationEnabled();
    r.pending = static_cast<unsigned>(pending_.size());
    r.version = algorithmVersionId(settings_.version);
    const auto &mode = algorithmMode({settings_.version, settings_.comparison});
    r.modeId = mode.id;
    r.comparisonFactor = mode.factor;
    r.motionStatus = motion_.status();
    r.movementAlgorithm = control::movement::algorithmId(resolvedMovementAlgorithm(settings_));
    r.heldTarget = held_;
    r.heldTargetName = heldName_;
    r.suspended = holding_ && !r.active;
    r.releaseReason = r.releaseRequired ? releaseReason_ : "";
    r.feedbackReference = feedbackReference_;
    r.lastPause = lastPause_;
    r.pauseCount = pauseCount_;
    if (lastPauseAtMs_ && now >= lastPauseAtMs_)
        r.lastPauseAgeMs = now - lastPauseAtMs_;
    if (in.frame && in.frame->local.mouseMapping && r.inputMappingReady)
    {
        r.yawPerCount = *in.frame->local.mouseMapping->yawDegreesPerDelta * countMapping.x;
        r.pitchPerCount = *in.frame->local.mouseMapping->pitchDegreesPerDelta * countMapping.y;
    }
    r.calibrating = calibration_.active();
    r.calibrationLeg = calibration_.leg();
    r.calibrationStatus = calibration_.status();
    r.calibrationResult = calibration_.result();
    return r;
}
} // namespace mousebox::games::cs2
