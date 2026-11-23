#include "output_program.hpp"
#include <algorithm>
#include <cmath>

namespace mousebox::games::valorant::aim
{
application::ports::ProgramStep OutputProgram::tick(const application::ports::OutputTick &tick)
{
    auto input = observe_();
    const std::lock_guard lock(mutex_);
    application::ports::ProgramStep out;
    proposal_.reset();
    const auto pause = [&](std::string reason) {
        clearMotion();
        block_ = std::move(reason);
        out.motion.stop = !stopped_;
        stopped_ = true;
        out.motion.event = control::MotionEvent::Suspended;
        return out;
    };
    if (generation_ && generation_ != tick.generation)
    {
        clearMotion();
        clearHold();
        released_ = false;
        releases_.reset();
    }
    generation_ = tick.generation;
    down_ = tick.buttonFresh && tick.down;
    const bool releasedBetween =
        tick.buttonFresh && tick.releaseCounter && releases_ && *tick.releaseCounter != *releases_;
    if (tick.buttonFresh && tick.releaseCounter)
        releases_ = tick.releaseCounter;
    if (!enabled_)
        return pause("disabled");
    if (!sourceRequested_)
        return pause("waiting-source");
    if (!tick.buttonFresh)
        return pause("button-stale");
    if (!tick.down || releasedBetween)
    {
        clearHold();
        released_ = true;
        return pause("waiting-side-key");
    }
    if (!released_ && !holding_)
        return pause("release-then-hold");
    if (!tick.limits.valid())
        return pause("device-unavailable");
    input.manualViewConfirmed = confirmed(input);
    if (input.frame && identity_ && input.frame->identity != *identity_)
    {
        clearHold();
        released_ = false;
        return pause("source-mismatch");
    }
    if (dead_)
        return pause("held-target-dead");
    const auto solution = solve(input, settings_, tick.nowMs, held_);
    if (!solution.ready())
    {
        if (solution.block == "held-target-dead")
            dead_ = true;
        return pause(solution.block);
    }
    const auto &frame = *input.frame;
    if (lastCapture_ && frame.cameraCapture < lastCapture_)
        return pause("clock-regressed");
    lastCapture_ = frame.cameraCapture;
    if (!held_)
        held_ = solution.target;
    identity_ = frame.identity;
    out.motion.begin = !holding_ || stopped_;
    holding_ = true;
    released_ = false;
    auto policy = control::speedPolicy(settings_.speed, tick.limits);
    policy.trajectory = settings_.trajectory;
    if (lastTick_ && tick.nowMs >= lastTick_ && tick.nowMs - lastTick_ < policy.intervalMs)
    {
        out.active = active_;
        return out;
    }
    if (lastTick_ && (tick.nowMs < lastTick_ || tick.nowMs - lastTick_ > 100))
        return pause("scheduler-stall");
    const auto dt = lastTick_ ? static_cast<double>(tick.nowMs - lastTick_) / 1000 : policy.intervalMs / 1000.0;
    lastTick_ = tick.nowMs;
    const auto change = [](control::movement::Vector a, control::movement::Vector b) {
        return std::hypot(std::remainder(a.x - b.x, 360.0), a.y - b.y);
    };
    const auto threshold =
        std::max(.001, std::min(std::abs(settings_.yawPerCount), std::abs(settings_.pitchPerCount)) * .25);
    if (awaitFeedback_)
    {
        if (change(frame.viewDegrees, *awaitFeedback_) <= threshold)
            return pause("feedback-timeout");
        awaitFeedback_.reset();
        feedback_.reset();
        unresponsiveSince_ = 0;
        unconfirmedDegrees_ = 0;
    }
    if (feedbackCapture_ != frame.cameraCapture)
    {
        if (feedback_ && change(frame.viewDegrees, *feedback_) > threshold)
        {
            unresponsiveSince_ = 0;
            unconfirmedDegrees_ = 0;
        }
        feedback_ = frame.viewDegrees;
        feedbackCapture_ = frame.cameraCapture;
    }
    if (unresponsiveSince_ && tick.nowMs >= unresponsiveSince_ && tick.nowMs - unresponsiveSince_ > 500 &&
        unconfirmedDegrees_ > .25)
    {
        awaitFeedback_ = frame.viewDegrees;
        return pause("feedback-timeout");
    }
    pending_.retire(solution.sampleMs, settings_.responseMs);
    if (pending_.expired(tick.nowMs))
        return pause("feedback-delayed");
    control::movement::Input movement;
    movement.errorDegrees = pending_.residual(solution.errorDegrees);
    movement.degreesPerCount = {settings_.yawPerCount, settings_.pitchPerCount};
    movement.dt = dt;
    movement.policy = policy;
    movement.now = tick.nowMs;
    const auto next = motion_.step(settings_.algorithm, movement, human_);
    out.motion.dx = next.counts.x;
    out.motion.dy = next.counts.y;
    out.motion.spent = static_cast<unsigned>(std::abs(next.counts.x) + std::abs(next.counts.y));
    out.sourceRevision = ++token_;
    out.active = true;
    active_ = true;
    block_ = "tracking";
    if (out.motion.spent)
    {
        const control::movement::Vector degrees{next.counts.x * settings_.yawPerCount,
                                                next.counts.y * settings_.pitchPerCount};
        const auto magnitude = std::hypot(degrees.x, degrees.y);
        pending_.add(token_, tick.nowMs, degrees);
        proposal_ = Proposal{token_, tick.nowMs, *held_, frame.identity, magnitude, degrees};
        if (!unresponsiveSince_)
            unresponsiveSince_ = tick.nowMs;
        unconfirmedDegrees_ += magnitude;
        stopped_ = false;
    }
    return out;
}
} // namespace mousebox::games::valorant::aim
