#include "count_calibration.hpp"
#include <algorithm>
#include <cmath>
namespace mousebox::games::cs2
{
void CountCalibrationSession::start()
{
    *this = {};
    active_ = true;
    status_ = "release-then-hold";
}
void CountCalibrationSession::cancel(std::string reason)
{
    active_ = false;
    result_.reset();
    status_ = std::move(reason);
}
control::MotionStep CountCalibrationSession::step(const ControlInput &in, const application::ports::OutputTick &t,
                                                  const AimSettings &s)
{
    const auto fail = [&](const char *why) {
        cancel(why);
        return control::MotionStep{false, true, 0, 0, 0, control::MotionEvent::EligibilityLost};
    };
    if (!active_)
        return {};
    if (!t.buttonFresh)
        return fail("button-stale");
    if (!t.down)
    {
        if (started_)
            return fail("released");
        released_ = true;
        status_ = "hold-side-key";
        return {};
    }
    if (!released_)
        return {};
    const auto observation = observeCalibration(in, s, t.nowMs);
    if (!observation.ready())
        return fail(observation.block.c_str());
    const auto &f = *in.frame;
    const auto &m = *observation.mapping;
    const auto identity = f.instance + ":" + std::to_string(f.epoch) + ":" + std::to_string(*f.local.pawn);
    if (!identity_.empty() && identity_ != identity)
        return fail("context-changed");
    identity_ = identity;
    if (!started_)
        started_ = phaseAt_ = t.nowMs;
    if (t.nowMs < started_ || t.nowMs - started_ > 15000 || !t.limits.valid())
        return fail("timeout");
    const bool horizontal = leg_ < 2;
    const auto a = *observation.angles;
    const double observed = horizontal ? a.y : a.x, cross = horizontal ? a.x : a.y;
    const double derivative = horizontal ? *m.yawDegreesPerDelta : *m.pitchDegreesPerDelta;
    const double other = horizontal ? *m.pitchDegreesPerDelta : *m.yawDegreesPerDelta;
    if (phase_ && (std::abs(derivative / derivative_ - 1) > .001 || std::abs(other / otherDerivative_ - 1) > .001))
        return fail("mapping-changed");
    if (!phase_)
    {
        status_ = "settling";
        if (t.nowMs - phaseAt_ < 250 || f.sequence == capture_)
            return {};
        baseline_ = observed;
        crossBaseline_ = cross;
        derivative_ = derivative;
        otherDerivative_ = other;
        total_ = static_cast<int>(std::clamp(std::round(.5 / std::abs(derivative)), 4.0, 32.0));
        if (leg_ % 2)
            total_ = -total_;
        sent_ = 0;
        capture_ = f.sequence;
        phase_ = 1;
        lastSent_ = 0;
    }
    if (phase_ == 1)
    {
        status_ = "measuring";
        if (lastSent_ && (t.nowMs < lastSent_ || t.nowMs - lastSent_ > 100))
            return fail("scheduler-stall");
        if (lastSent_ && t.nowMs - lastSent_ < t.limits.minimumIntervalMs)
            return {};
        const int limit = static_cast<int>(std::min(t.limits.maximumStepCounts, t.limits.maximumBudget / 2));
        const int count = std::clamp(total_ - sent_, -limit, limit);
        if (!sent_)
            firstSent_ = t.nowMs;
        sent_ += count;
        lastSent_ = t.nowMs;
        if (sent_ == total_)
        {
            phase_ = 2;
            phaseAt_ = t.nowMs;
        }
        return {false, false, horizontal ? count : 0, horizontal ? 0 : count};
    }
    if (phase_ == 2)
    {
        status_ = "waiting-feedback";
        if (f.sequence == capture_ ||
            observation.timing.localSampleMs - observation.timing.uncertaintyMs < static_cast<double>(lastSent_ + 250))
            return {};
        const auto change = std::remainder(observed - baseline_, 360.0);
        const auto crossChange = std::remainder(cross - crossBaseline_, 360.0);
        const double ratio = change / (total_ * derivative_);
        if (!std::isfinite(ratio) || std::abs(ratio) < .05 || std::abs(ratio) > 20 || std::abs(change) < .1 ||
            std::abs(change) > 15 || std::abs(crossChange) > std::max(.15, std::abs(change) * .1))
            return fail("motion-not-isolated");
        responseUpper_ =
            std::max(responseUpper_, std::clamp(observation.timing.localSampleMs - firstSent_, 0.0, 500.0));
        ratios_[leg_++] = ratio;
        if (leg_ == 4)
        {
            if (std::abs(ratios_[0] - ratios_[1]) > std::abs(ratios_[0]) * .1 ||
                std::abs(ratios_[2] - ratios_[3]) > std::abs(ratios_[2]) * .1)
                return fail("nonlinear-response");
            result_ = CountCalibration{f.build,
                                       (ratios_[0] + ratios_[1]) / 2,
                                       (ratios_[2] + ratios_[3]) / 2,
                                       *m.yawDegreesPerDelta,
                                       *m.pitchDegreesPerDelta,
                                       responseUpper_};
            active_ = false;
            status_ = "complete-not-saved";
            return {false, true};
        }
        phase_ = 0;
        phaseAt_ = t.nowMs;
        capture_ = f.sequence;
    }
    return {};
}
} // namespace mousebox::games::cs2
