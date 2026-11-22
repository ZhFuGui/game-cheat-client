#include "direct.hpp"
#include <algorithm>
#include <cmath>
namespace mousebox::control::movement
{
Result Direct::step(const Input &in, HumanMotionRuntime &human)
{
    const auto learned = human.limits();
    const double speed =
        human.enabled() ? std::min(in.policy.maximumCountsPerSecond, learned.speed) : in.policy.maximumCountsPerSecond;
    const auto axis = [&](double error, double scale, double &tokens) {
        tokens = std::min(static_cast<double>(in.policy.maximumStepCounts), tokens + speed * in.dt);
        if (std::abs(error) <= std::max(.01, std::abs(scale) * .5))
            return 0;
        // Quantize the absolute residual once. Its sub-count error remains in
        // the next current angle, rather than repeatedly accumulating old data.
        const double available = std::floor(tokens);
        const int count = static_cast<int>(std::round(std::clamp(error / scale, -available, available)));
        tokens -= std::abs(count);
        return count;
    };
    Result result;
    result.counts = {axis(in.errorDegrees.x, in.degreesPerCount.x, tokensX_),
                     axis(in.errorDegrees.y, in.degreesPerCount.y, tokensY_)};
    result.status = "direct";
    return result;
}
} // namespace mousebox::control::movement
