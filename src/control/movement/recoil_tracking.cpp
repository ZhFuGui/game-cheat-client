#include "recoil_tracking.hpp"
#include <algorithm>
#include <cmath>
namespace mousebox::control::movement
{
Result RecoilTracking::step(const Input &in, HumanMotionRuntime &human)
{
    if (!in.recoilEnabled || !in.recoilDeltaQualified)
    {
        previous_.reset();
        debt_ = {};
        tokensX_ = tokensY_ = 0;
        auto result = tracking_.step(in, human);
        result.status = in.recoilEnabled ? "branch-baseline" : "recoil-disabled";
        return result;
    }
    bool rebased = !previous_;
    if (previous_ && in.capture != previous_->capture)
    {
        const double simulationMs = (in.simulationTicks - previous_->simulationTicks) * in.secondsPerTick * 1000;
        const double sourceMs = static_cast<double>(in.sourceStartNs - previous_->sourceStartNs) / 1000000;
        // Native tick resolution plus both observations' timing uncertainty.
        // This only resets the fast differential baseline; it never ends a hold.
        const double resolution = in.secondsPerTick * 1000 + in.uncertaintyMs + previous_->uncertaintyMs;
        rebased = in.capture < previous_->capture || simulationMs < 0 || sourceMs < 0 ||
                  std::abs(simulationMs - sourceMs) > resolution;
    }
    if (rebased)
        debt_ = {};
    else
    {
        debt_.y -= in.recoilDegrees.y - previous_->recoilDegrees.y;
        const double delta = in.recoilDegrees.x - previous_->recoilDegrees.x;
        debt_.x -= in.recoilHorizontalPeriod > 0 ? std::remainder(delta, in.recoilHorizontalPeriod) : delta;
    }
    previous_ = in;
    const auto useful = [](double debt, double error) {
        // Never send a fast correction against the complete current error or
        // retain a hidden excess that could arrive after a manual/target change.
        return debt * error > 0 ? std::copysign(std::min(std::abs(debt), std::abs(error)), error) : 0.0;
    };
    debt_.y = useful(debt_.y, in.errorDegrees.y);
    debt_.x = useful(debt_.x, in.errorDegrees.x);
    const auto learned = human.limits();
    const double speed =
        human.enabled() ? std::min(in.policy.maximumCountsPerSecond, learned.speed) : in.policy.maximumCountsPerSecond;
    const auto budget = [&](double &tokens) {
        tokens = std::min(static_cast<double>(in.policy.maximumStepCounts), tokens + speed * in.dt);
        return static_cast<int>(std::floor(tokens));
    };
    const Counts available{budget(tokensX_), budget(tokensY_)};
    const auto fast = [](double debt, double scale, int maximum) {
        return static_cast<int>(
            std::trunc(std::clamp(debt / scale, -static_cast<double>(maximum), static_cast<double>(maximum))));
    };
    Result result;
    result.fast = {fast(debt_.x, in.degreesPerCount.x, available.x), fast(debt_.y, in.degreesPerCount.y, available.y)};
    auto residual = in;
    residual.errorDegrees.y -= debt_.y;
    residual.errorDegrees.x -= debt_.x;
    const Counts remaining{available.x - std::abs(result.fast.x), available.y - std::abs(result.fast.y)};
    const auto smooth = tracking_.step(residual, human, remaining).counts;
    result.counts = {result.fast.x + smooth.x, result.fast.y + smooth.y};
    tokensX_ -= std::abs(result.counts.x);
    tokensY_ -= std::abs(result.counts.y);
    debt_.y -= result.fast.y * in.degreesPerCount.y;
    debt_.x -= result.fast.x * in.degreesPerCount.x;
    result.status = rebased ? "time-rebased" : result.fast.x || result.fast.y ? "fast-recoil" : "tracking";
    return result;
}
} // namespace mousebox::control::movement
