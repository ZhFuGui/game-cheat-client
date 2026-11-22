#include "games/pubg/pipeline/motion_planner.hpp"
#include "games/pubg/pipeline/latency_prediction.hpp"
#include "games/pubg/pipeline/native_aim.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace mousebox::control
{
MotionPlanner::MotionPlanner(MotionPolicy p) : policy_(p)
{
    if (!std::isfinite(p.gain) || p.gain <= 0 || !std::isfinite(p.maximumCountsPerSecond) ||
        p.maximumCountsPerSecond <= 0 || p.intervalMs < 2 || p.intervalMs > 30 || !p.maximumStepCounts ||
        p.maximumStepCounts > 32767 || p.maximumCountsPerSecond > 1000.0 * p.maximumStepCounts / p.intervalMs ||
        !std::isfinite(p.acceleration) || p.acceleration <= 0 || !std::isfinite(p.deadZoneDegrees) ||
        !p.trajectory.valid() || p.deadZoneDegrees < 0 || p.budget > 4096 || p.durationMs > 5000 ||
        !std::isfinite(p.maximumJerk) || p.maximumJerk < 0)
        throw std::invalid_argument("Invalid motion policy");
}
void MotionPlanner::reset() noexcept
{
    active_ = false;
    settled_ = false;
    trajectory_.reset();
    spent_ = 0;
    human_.reset();
    feedbackSeen_ = false;
    viewStartNs_ = viewFinishNs_ = 0;
    pendingYaw_ = pendingPitch_ = 0;
    pending_.clear();
    responseAligned_ = creditedLastStep_ = false;
    responseSourceNs_ = 0;
    responseLocalMs_ = 0;
    unresponsiveSince_ = 0;
    sourceTimed_ = tracking_ = false;
    residualTimeMs_ = disturbanceYawRate_ = disturbancePitchRate_ = 0;
    disturbanceYawLead_ = disturbancePitchLead_ = 0;
    residualReady_ = false;
    feedbackTiming_ = {};
    angularResiduals_.clear();
    pointSourceNs_ = 0;
    pointObservedMs_ = 0;
    previousPoint_ = pointVelocity_ = {};
    resolvedContext_.reset();
    nativeReference_ = false;
    pendingInputYaw_ = 0;
}
bool MotionPlanner::reconcile(const Frame &view, const Calibration &c, std::uint64_t now,
                              const NativeAimReference *native)
{
    const auto value = native ? directionCoordinates(native->direction) : view.rotation;
    if (!value || !finite(*value))
        return false;
    const auto rotation = *value; // Native path stores only spherical pitch/yaw; never infers or consumes roll.
    const auto sourceFinish = native                            ? native->sourceFinishNs
                              : view.rotationSourceFinishNs > 0 ? view.rotationSourceFinishNs
                                                                : view.header.sourceFinishNs;
    const auto sourceStart = native                            ? native->sourceStartNs
                             : view.rotationSourceFinishNs > 0 ? view.rotationSourceStartNs
                                                               : sourceFinish;
    const auto sample = estimateSampleTime(sourceStart, sourceFinish, view.header.sourceClock, now);
    if (native && !sample.usable())
        return false;
    if (native && feedbackSeen_ && (sourceFinish < viewFinishNs_ || sourceStart < viewStartNs_))
        return false;
    feedbackTiming_ = sample;
    const bool previouslyTimed = sourceTimed_;
    sourceTimed_ = sample.usable();
    if (!sourceTimed_)
    {
        residualTimeMs_ = disturbanceYawRate_ = disturbancePitchRate_ = 0;
        residualReady_ = false;
        angularResiduals_.clear();
    }
    if (!feedbackSeen_)
    {
        feedbackSeen_ = true;
        viewStartNs_ = sourceStart;
        viewFinishNs_ = sourceFinish;
        observedRotation_ = rotation;
        return true;
    }
    if (sourceFinish > viewFinishNs_)
    {
        const double yaw = std::remainder(rotation.y - observedRotation_.y, 360.0);
        const double pitch = std::remainder(rotation.x - observedRotation_.x, 360.0);
        if (std::abs(yaw) >= std::abs(c.yawDegreesPerCount) * 0.25 ||
            std::abs(pitch) >= std::abs(c.pitchDegreesPerCount) * 0.25)
            unresponsiveSince_ = 0;
        if (sample.usable())
        {
            // Source timing, not recoil-corrupted angle-prefix matches, decides
            // which command effects are expected in this observation. 20ms is
            // a declared response model, not a measured device acknowledgement.
            const double reflectedBefore = sample.localSampleMs - AngularFeedbackTuning::responseMs;
            double ownYaw = 0, ownPitch = 0;
            while (!pending_.empty() && pending_.front().at <= reflectedBefore)
            {
                ownYaw += pending_.front().yaw;
                ownPitch += pending_.front().pitch;
                pending_.pop_front();
            }
            // Differentiate source intervals, not receive times or clock-probe
            // updates. A repeated sample must never become another impulse.
            const double seconds =
                (static_cast<double>(sourceStart - viewStartNs_) + static_cast<double>(sourceFinish - viewFinishNs_)) /
                2e9;
            if (tracking_ && previouslyTimed && seconds >= 0.002 && seconds <= 0.15)
            {
                constexpr auto rateLimit = AngularFeedbackTuning::maximumRateDegreesPerSecond;
                const double yawRate = std::clamp((yaw - ownYaw) / seconds, -rateLimit, rateLimit);
                const double pitchRate = std::clamp((pitch - ownPitch) / seconds, -rateLimit, rateLimit);
                residualTimeMs_ += seconds * 1000;
                // Residual contains manual motion, camera effects and model
                // error as well as recoil. It is neither pure recoil nor ACK.
                angularResiduals_.push_back({residualTimeMs_, seconds * 1000, yawRate * seconds, pitchRate * seconds});
                const double oldest = residualTimeMs_ - AngularFeedbackTuning::windowMs;
                while (!angularResiduals_.empty() && angularResiduals_.front().at <= oldest)
                    angularResiduals_.pop_front();
                double elapsed = 0, sumYaw = 0, sumPitch = 0;
                for (const auto &item : angularResiduals_)
                {
                    const double fraction = std::clamp((item.at - oldest) / item.span, 0.0, 1.0);
                    elapsed += item.span * fraction;
                    sumYaw += item.yaw * fraction;
                    sumPitch += item.pitch * fraction;
                }
                residualReady_ = elapsed >= AngularFeedbackTuning::minimumWindowMs;
                disturbanceYawRate_ = residualReady_ ? sumYaw * 1000 / elapsed : 0;
                disturbancePitchRate_ = residualReady_ ? sumPitch * 1000 / elapsed : 0;
                // Do not amplify quantization/model noise into near-head motion.
                const auto suppressNoise = [&](double rate, double degreesPerCount) {
                    const double noise = std::max(AngularFeedbackTuning::minimumRateDegreesPerSecond,
                                                  2 * std::abs(degreesPerCount) * 1000 / std::max(1.0, elapsed));
                    return std::abs(rate) >= noise ? rate : 0;
                };
                disturbanceYawRate_ = suppressNoise(disturbanceYawRate_, c.yawDegreesPerCount);
                disturbancePitchRate_ = suppressNoise(disturbancePitchRate_, c.pitchDegreesPerCount);
            }
            else
            {
                residualTimeMs_ = disturbanceYawRate_ = disturbancePitchRate_ = 0;
                residualReady_ = false;
                angularResiduals_.clear();
            }
            responseAligned_ = false;
        }
        if (!sample.usable() && !responseAligned_ && !pending_.empty())
        {
            const bool horizontal = std::abs(yaw / c.yawDegreesPerCount) >= std::abs(pitch / c.pitchDegreesPerCount);
            const double change = horizontal ? yaw : pitch,
                         scale = std::abs(horizontal ? c.yawDegreesPerCount : c.pitchDegreesPerCount);
            double accumulated = 0, other = 0, best = scale * 0.5;
            std::optional<std::size_t> matched;
            double approximateScore = std::numeric_limits<double>::infinity();
            std::optional<std::size_t> approximate;
            if (std::abs(change) >= scale * 0.5)
                for (std::size_t i = 0; i < pending_.size(); ++i)
                {
                    const auto &entry = pending_[i];
                    const double part = horizontal ? entry.yaw : entry.pitch;
                    if (part * change < 0)
                    {
                        matched.reset();
                        break;
                    }
                    accumulated += part;
                    other += horizontal ? entry.pitch : entry.yaw;
                    const double error = std::abs(accumulated - change);
                    const double crossError = std::abs(other - (horizontal ? pitch : yaw));
                    const double crossScale = std::abs(horizontal ? c.pitchDegreesPerCount : c.yawDegreesPerCount);
                    if (error <= best && crossError <= crossScale * 2 && i + 1 < pending_.size())
                    {
                        best = error;
                        matched = i;
                    }
                    // Larger integer steps can skip an exact prefix match when a formula differs slightly from real
                    // response. Accept only a bounded, same-direction scale and consistent second axis; never infer
                    // effects from a zero camera change.
                    if (std::abs(change) >= scale && accumulated * change > 0 && i + 1 < pending_.size())
                    {
                        const double ratio = change / accumulated, second = horizontal ? pitch : yaw;
                        const double mismatch = std::abs(other * ratio - second);
                        const double score = std::abs(1 - ratio) + mismatch / crossScale;
                        if (ratio >= 0.75 && ratio <= 1.25 && mismatch <= crossScale * 2 && score < approximateScore)
                        {
                            approximateScore = score;
                            approximate = i;
                        }
                    }
                }
            if (!matched)
                matched = approximate;
            if (matched)
            {
                // Infer only a response phase from a bracketed command prefix. This is not clock mapping or a device
                // ACK.
                responseLocalMs_ = (static_cast<double>(pending_[*matched].at) + pending_[*matched + 1].at) / 2;
                responseSourceNs_ = sourceFinish;
                responseAligned_ = true;
            }
            else
            {
                const auto confirm = [&](double changeValue, bool horizontalAxis) {
                    for (auto &entry : pending_)
                    {
                        auto &part = horizontalAxis ? entry.yaw : entry.pitch;
                        if (part * changeValue <= 0)
                            continue;
                        const double used = std::copysign(std::min(std::abs(changeValue), std::abs(part)), part);
                        part -= used;
                        changeValue -= used;
                    }
                };
                confirm(yaw, true);
                confirm(pitch, false);
                for (auto &entry : pending_)
                {
                    if (std::abs(entry.yaw) < std::abs(c.yawDegreesPerCount) * 0.25)
                        entry.yaw = 0;
                    if (std::abs(entry.pitch) < std::abs(c.pitchDegreesPerCount) * 0.25)
                        entry.pitch = 0;
                }
                while (!pending_.empty() && !pending_.front().yaw && !pending_.front().pitch)
                    pending_.pop_front();
            }
        }
        if (!sample.usable() && responseAligned_)
        {
            // Advance using source-domain elapsed time, so recoil/manual motion cannot masquerade as command
            // acknowledgements.
            const double phase = responseLocalMs_ + static_cast<double>(sourceFinish - responseSourceNs_) / 1000000;
            if (phase > static_cast<double>(now) + 30)
                return false;
            while (!pending_.empty() && pending_.front().at <= phase)
                pending_.pop_front();
        }
        observedRotation_ = rotation;
        viewStartNs_ = sourceStart;
        viewFinishNs_ = sourceFinish;
    }
    const double horizon = sample.usable() ? (sample.ageMs + AngularFeedbackTuning::responseMs) / 1000 : 0;
    constexpr auto leadLimit = AngularFeedbackTuning::maximumLeadDegrees;
    disturbanceYawLead_ = std::clamp(disturbanceYawRate_ * horizon, -leadLimit, leadLimit);
    disturbancePitchLead_ = std::clamp(disturbancePitchRate_ * horizon, -leadLimit, leadLimit);
    pendingYaw_ = pendingPitch_ = pendingInputYaw_ = 0;
    for (const auto &entry : pending_)
    {
        pendingYaw_ += entry.yaw;
        pendingPitch_ += entry.pitch;
        pendingInputYaw_ += entry.inputYaw;
    }
    // Never repeatedly resend a displacement just because old camera observations still show the old error.
    return pending_.size() <= 256 && (pending_.empty() || now - pending_.front().at <= 800) &&
           (!unresponsiveSince_ || now - unresponsiveSince_ <= 800);
}
void MotionPlanner::rejectStep(const MotionStep &step) noexcept
{
    trajectory_.reject();
    if (creditedLastStep_ && !pending_.empty())
    {
        pendingYaw_ -= pending_.back().yaw;
        pendingPitch_ -= pending_.back().pitch;
        pendingInputYaw_ -= pending_.back().inputYaw;
        pending_.pop_back();
        creditedLastStep_ = false;
        const auto cost = static_cast<unsigned>(std::abs(step.dx) + std::abs(step.dy));
        spent_ -= std::min(spent_, cost);
        if (pending_.empty())
            unresponsiveSince_ = 0;
    }
}
MotionStep MotionPlanner::cancel(MotionEvent event) noexcept
{
    const bool stop = active_;
    const unsigned spent = spent_;
    reset();
    released_ = false;
    stoppedBy_ = event;
    return {false, stop, 0, 0, spent, event};
}
MotionStep MotionPlanner::step(bool down, std::uint64_t generation, std::uint64_t now, std::optional<Target> target,
                               const Calibration &c, bool eligible, const Frame *view)
{
    creditedLastStep_ = false;
    const bool valid = eligible && target && validCalibration(c) && std::isfinite(target->error.horizontal) &&
                       std::isfinite(target->error.vertical) && target->identity.id != 0;
    const auto native = target && target->nativeAim ? &*target->nativeAim : nullptr;
    const auto context =
        target && target->resolvedAim
            ? std::optional(std::pair{target->resolvedAim->weapon, target->resolvedAim->modelRevision +
                                                                       (native ? "/native/" + native->context : "")})
            : std::nullopt;
    const bool resolvedValid = !target || !target->resolvedAim ||
                               (view && finite(target->resolvedAim->position) && finite(target->resolvedAim->origin) &&
                                (!target->resolvedAim->nextPosition || finite(*target->resolvedAim->nextPosition)));
    const bool nativeValid =
        !native ||
        (view && target->resolvedAim && target->resolvedAim->launchDirection &&
         directionCoordinates(native->direction) &&
         nativeInputError(native->direction, *target->resolvedAim->launchDirection, native->controllerYawDegrees) &&
         estimateSampleTime(native->sourceStartNs, native->sourceFinishNs, view->header.sourceClock, now).usable() &&
         (!target->resolvedAim->nextLaunchDirection ||
          nativeInputError(native->direction, *target->resolvedAim->nextLaunchDirection,
                           native->controllerYawDegrees)));
    if (!down)
    {
        const bool stop = active_;
        reset();
        released_ = true;
        stoppedBy_ = MotionEvent::None;
        return {false, stop, 0, 0, 0, stop ? MotionEvent::Released : MotionEvent::None};
    }
    if (active_ &&
        (!valid || !resolvedValid || !nativeValid || (native != nullptr) != nativeReference_ ||
         generation != generation_ || context != resolvedContext_ || target->identity != target_ || now < last_ ||
         (policy_.durationMs && now >= deadline_) || now - last_ > 30 || (policy_.budget && spent_ >= policy_.budget)))
    {
        const auto reason = !valid || !resolvedValid || !nativeValid || (native != nullptr) != nativeReference_
                                ? MotionEvent::EligibilityLost
                            : generation != generation_ || context != resolvedContext_ ? MotionEvent::ContextChanged
                            : target->identity != target_                              ? MotionEvent::TargetChanged
                            : now < last_                                              ? MotionEvent::ClockRegressed
                            : policy_.durationMs && now >= deadline_                   ? MotionEvent::TimeLimit
                            : now - last_ > 30                                         ? MotionEvent::SchedulerStall
                                                                                       : MotionEvent::CountLimit;
        const auto spent = spent_;
        reset();
        released_ = false;
        stoppedBy_ = reason;
        return {false, true, 0, 0, spent, reason};
    }
    if (!active_)
    {
        if (!released_)
            return {};
        released_ = false; // A rejected press is consumed; later data cannot start it while held.
        if (!valid || !resolvedValid || !nativeValid || !generation ||
            now > std::numeric_limits<std::uint64_t>::max() - policy_.durationMs)
        {
            stoppedBy_ = MotionEvent::RejectedPress;
            return {false, false, 0, 0, 0, MotionEvent::RejectedPress};
        }
        active_ = true;
        generation_ = generation;
        target_ = target->identity;
        resolvedContext_ = context;
        nativeReference_ = native != nullptr;
        deadline_ = now + policy_.durationMs;
        last_ = now;
        if (view)
            reconcile(*view, c, now, native);
        stoppedBy_ = MotionEvent::None;
        return {true, false, 0, 0, 0, MotionEvent::Begin};
    }
    bool estimated = false;
    double targetVx = 0, targetVy = 0;
    Vec3 nativePredicted{};
    double nativeControllerYaw = 0;
    if (view && native)
    {
        if (!reconcile(*view, c, now, native))
            return cancel(MotionEvent::CameraUnconfirmed);
        const auto angles = *directionCoordinates(native->direction);
        nativePredicted = coordinateDirection(angles.x + pendingPitch_ + (tracking_ ? disturbancePitchLead_ : 0),
                                              angles.y + pendingYaw_ + (tracking_ ? disturbanceYawLead_ : 0));
        nativeControllerYaw = native->controllerYawDegrees + pendingInputYaw_;
        auto desired = *target->resolvedAim->launchDirection;
        if (target->resolvedAim->nextLaunchDirection)
        {
            const auto current = nativeInputError(nativePredicted, desired, nativeControllerYaw);
            const auto future =
                nativeInputError(nativePredicted, *target->resolvedAim->nextLaunchDirection, nativeControllerYaw);
            if (!current || !future)
                return cancel(MotionEvent::EligibilityLost);
            if (policy_.maximumJerk)
            {
                const double yaw = (future->horizontal - current->horizontal) / 0.02,
                             pitch = (future->vertical - current->vertical) / 0.02;
                if (std::hypot(yaw, pitch) >= 1)
                {
                    targetVx = yaw / c.yawDegreesPerCount;
                    targetVy = pitch / c.pitchDegreesPerCount;
                }
            }
            if (tracking_ && sourceTimed_)
                desired = *target->resolvedAim->nextLaunchDirection;
        }
        const auto error = nativeInputError(nativePredicted, desired, nativeControllerYaw);
        if (!error)
            return cancel(MotionEvent::EligibilityLost);
        const auto distance = target->error.distanceUnits;
        target->error = *error;
        target->error.distanceUnits = distance;
        estimated = true;
        pointVelocity_ = {};
        pointSourceNs_ = 0;
    }
    else if (view && ((target->position && target->origin) || target->resolvedAim))
    {
        if (!reconcile(*view, c, now))
        {
            const auto spent = spent_;
            reset();
            released_ = false;
            stoppedBy_ = MotionEvent::CameraUnconfirmed;
            return {false, true, 0, 0, spent, MotionEvent::CameraUnconfirmed};
        }
        auto predicted = *view;
        predicted.rotation->y += pendingYaw_ + (tracking_ ? disturbanceYawLead_ : 0);
        predicted.rotation->x += pendingPitch_ + (tracking_ ? disturbancePitchLead_ : 0);
        auto point = target->resolvedAim ? target->resolvedAim->position : *target->position;
        const auto origin = target->resolvedAim ? target->resolvedAim->origin : *target->origin;
        const auto positionPrediction = target->resolvedAim ? PositionPrediction{} : predictPosition(*target, now);
        if (target->resolvedAim)
        {
            if (policy_.maximumJerk && target->resolvedAim->nextPosition)
            {
                const auto current = angleTo(predicted, point, origin),
                           future = angleTo(predicted, *target->resolvedAim->nextPosition, origin);
                if (current && future)
                {
                    const double yaw = std::remainder(future->horizontal - current->horizontal, 360.0) / 0.02,
                                 pitch = (future->vertical - current->vertical) / 0.02;
                    if (std::hypot(yaw, pitch) >= 1)
                    {
                        targetVx = yaw / c.yawDegreesPerCount;
                        targetVy = pitch / c.pitchDegreesPerCount;
                    }
                }
                // The existing +20ms ballistic solution is also the target
                // reference at the nominal input-effect time. Do not apply
                // another bone-age or bullet-flight compensation to this point.
                if (tracking_ && sourceTimed_)
                    point = *target->resolvedAim->nextPosition;
            }
            pointVelocity_ = {};
            pointSourceNs_ = 0;
        }
        else if (positionPrediction.applied())
        {
            point = positionPrediction.position;
            const auto velocity = target->bodyWorldVelocity->value;
            const Vec3 ahead{point.x + velocity.x * 0.02, point.y + velocity.y * 0.02, point.z + velocity.z * 0.02};
            const auto current = angleTo(predicted, point, *target->origin),
                       future = angleTo(predicted, ahead, *target->origin);
            if (policy_.maximumJerk && current && future)
            {
                const double yaw = std::remainder(future->horizontal - current->horizontal, 360.0) / 0.02;
                const double pitch = (future->vertical - current->vertical) / 0.02;
                if (std::hypot(yaw, pitch) >= 1)
                {
                    targetVx = yaw / c.yawDegreesPerCount;
                    targetVy = pitch / c.pitchDegreesPerCount;
                }
            }
            pointVelocity_ = {};
            pointSourceNs_ = 0; // No extra local lookahead or stale differenced velocity.
        }
        else if (!target->nativeMovementExpected && policy_.maximumJerk && target->sourceFinishNs > 0)
        {
            const Vec3 world{point.x + target->origin->x, point.y + target->origin->y, point.z + target->origin->z};
            if (target->sourceFinishNs != pointSourceNs_)
            {
                const double elapsed = static_cast<double>(target->sourceFinishNs - pointSourceNs_) / 1e9;
                if (pointSourceNs_ && elapsed >= 0.002 && elapsed <= 0.15)
                {
                    const Vec3 measured{(world.x - previousPoint_.x) / elapsed, (world.y - previousPoint_.y) / elapsed,
                                        (world.z - previousPoint_.z) / elapsed};
                    if (std::hypot(measured.x, measured.y, measured.z) < 1e-6)
                        pointVelocity_ = {};
                    else
                        pointVelocity_ = {0.65 * measured.x + 0.35 * pointVelocity_.x,
                                          0.65 * measured.y + 0.35 * pointVelocity_.y,
                                          0.65 * measured.z + 0.35 * pointVelocity_.z};
                }
                else
                    pointVelocity_ = {};
                previousPoint_ = world;
                pointSourceNs_ = target->sourceFinishNs;
                pointObservedMs_ = now;
            }
            if (now >= pointObservedMs_ && now - pointObservedMs_ <= 150)
            {
                // Estimate only actual Living-point motion. Camera rotation/our own commands are not target velocity.
                // At most 35ms of local observation age plus a short 25ms planning lookahead.
                // This is bounded motion prediction, not a claimed cross-machine source age.
                const double age = 0.025 + std::min(0.035, static_cast<double>(now - pointObservedMs_) / 1000);
                const Vec3 ahead{point.x + pointVelocity_.x * 0.02, point.y + pointVelocity_.y * 0.02,
                                 point.z + pointVelocity_.z * 0.02};
                const auto current = angleTo(predicted, point, *target->origin),
                           future = angleTo(predicted, ahead, *target->origin);
                if (current && future)
                {
                    const double yaw = std::remainder(future->horizontal - current->horizontal, 360.0) / 0.02;
                    const double pitch = (future->vertical - current->vertical) / 0.02;
                    if (std::hypot(yaw, pitch) >= 1)
                    {
                        targetVx = yaw / c.yawDegreesPerCount;
                        targetVy = pitch / c.pitchDegreesPerCount;
                        point = {point.x + pointVelocity_.x * age, point.y + pointVelocity_.y * age,
                                 point.z + pointVelocity_.z * age};
                    }
                }
            }
            else
                pointVelocity_ = {};
        }
        if (const auto error = angleTo(predicted, point, origin))
        {
            target->error = *error;
            estimated = true;
        }
    }
    if (now - last_ < policy_.intervalMs)
        return {false, false, 0, 0, spent_};
    const double dt = static_cast<double>(now - last_) / 1000;
    last_ = now;
    const double angularDistance = std::hypot(target->error.horizontal, target->error.vertical);
    // Keep the near-target tolerance comparable on screen while zooming; a fixed angular zone is huge at narrow FOV.
    // Existing measured-mode tuning is preserved. The estimate's current FOV scales both axes consistently.
    const double zoomScale = c.formulaEstimated ? std::min(1.0, c.fov / 80.0) : 1.0;
    const double deadZone = policy_.deadZoneDegrees * zoomScale;
    const double enter = std::max(0.06 * zoomScale, deadZone), leave = std::max(0.12 * zoomScale, enter * 2);
    // Acquire along a smooth planned path, then keep a fast bounded servo for
    // continuing target motion and external angular disturbances. A recoil
    // impulse must not restart a slow acquisition path on every shot.
    if (estimated && sourceTimed_ && angularDistance <= std::max(0.2, 1.5 * zoomScale))
        tracking_ = true;
    if (!targetVx && !targetVy && (angularDistance <= enter || (settled_ && angularDistance < leave)))
    {
        settled_ = true;
        trajectory_.reset();
        human_.reset();
        return {false, false, 0, 0, spent_};
    }
    settled_ = false;
    movement::TrajectoryInput movementInput;
    movementInput.movement.errorDegrees = {target->error.horizontal, target->error.vertical};
    movementInput.movement.degreesPerCount = {c.yawDegreesPerCount, c.pitchDegreesPerCount};
    movementInput.movement.policy = policy_;
    movementInput.movement.dt = dt;
    movementInput.movement.now = now;
    movementInput.targetVelocity = {targetVx, targetVy};
    movementInput.deadZoneDegrees = deadZone;
    movementInput.trackingGain = AngularFeedbackTuning::trackingGain;
    movementInput.geometryAvailable = estimated;
    movementInput.tracking = tracking_ && sourceTimed_;
    const auto movement = trajectory_.step(movementInput, human_);
    if (!movement)
        return cancel(MotionEvent::EligibilityLost);
    int dx = movement->x, dy = movement->y;
    const auto take = [&](int count) {
        const unsigned requested = static_cast<unsigned>(std::abs(count));
        const unsigned amount = policy_.budget ? std::min(requested, policy_.budget - spent_) : requested;
        spent_ += std::min(amount, std::numeric_limits<unsigned>::max() - spent_);
        return count < 0 ? -static_cast<int>(amount) : static_cast<int>(amount);
    };
    dx = take(dx);
    dy = take(dy);
    if (estimated && (dx || dy))
    {
        PendingMotion motion{now, dx * c.yawDegreesPerCount, dy * c.pitchDegreesPerCount};
        if (native)
        {
            const auto before = directionCoordinates(nativePredicted),
                       after = directionCoordinates(
                           nativeInputEffect(nativePredicted, nativeControllerYaw, motion.yaw, motion.pitch));
            if (!before || !after)
                return cancel(MotionEvent::EligibilityLost);
            motion.inputYaw = motion.yaw;
            motion.yaw = std::remainder(after->y - before->y, 360.0);
            motion.pitch = after->x - before->x;
        }
        pending_.push_back(motion);
        pendingYaw_ += motion.yaw;
        pendingPitch_ += motion.pitch;
        pendingInputYaw_ += motion.inputYaw;
        creditedLastStep_ = true;
        if (!unresponsiveSince_)
            unresponsiveSince_ = now;
    }
    return {false, false, dx, dy, spent_};
}
} // namespace mousebox::control
