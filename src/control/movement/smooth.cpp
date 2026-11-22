#include "smooth.hpp"
#include <limits>
#include <algorithm>
#include <cmath>
namespace mousebox::control::movement
{
Result Smooth::step(const Input &in, HumanMotionRuntime &human, std::optional<Counts> caps, Factors factors)
{
    Result result;
    const double errorX = in.errorDegrees.x, errorY = in.errorDegrees.y;
    const double yaw = in.degreesPerCount.x, pitch = in.degreesPerCount.y, dt = in.dt;
    const auto &policy = in.policy;
    const auto limits = human.limits();
    const double speed = human.enabled() && factors.learnedLimits
                             ? std::min(policy.maximumCountsPerSecond, limits.speed)
                             : policy.maximumCountsPerSecond;
    const double acceleration = human.enabled() && factors.learnedLimits
                                    ? std::min({policy.acceleration, limits.acceleration, limits.braking})
                                    : policy.acceleration;
    const auto variation = factors.variation
                               ? human.offset(errorX / yaw, errorY / pitch, std::hypot(errorX, errorY), in.now)
                               : HumanVariation{};
    const auto axis = [&](double error, double scale, double offset, double &velocity, double &fraction,
                          unsigned maximumStep) {
        if (std::abs(error) <= std::max(.01, std::abs(scale) * .5))
        {
            velocity = fraction = 0;
            return 0;
        }
        const double remaining = error / scale + offset;
        const double desired = std::copysign(
            std::min({factors.gainLimit ? std::abs(remaining) * policy.gain : std::numeric_limits<double>::infinity(),
                      speed,
                      factors.braking ? std::sqrt(2 * acceleration * std::abs(remaining))
                                      : std::numeric_limits<double>::infinity()}),
            remaining);
        if (factors.acceleration)
            velocity += std::clamp(desired - velocity, -acceleration * dt, acceleration * dt);
        else
            velocity = desired;
        if (velocity * remaining < 0)
            velocity = 0;
        double amount = velocity * dt + fraction;
        amount = std::clamp(amount, -static_cast<double>(policy.maximumStepCounts),
                            static_cast<double>(policy.maximumStepCounts));
        amount = std::copysign(std::min(std::abs(amount), std::abs(remaining)), remaining);
        const int count = static_cast<int>(std::trunc(amount));
        fraction = amount - count;
        // Optional v2 budget clips whole unsent counts, preserving the baseline
        // fractional accumulator even for sub-count speeds. V1 uses its full cap.
        return std::clamp(count, -static_cast<int>(maximumStep), static_cast<int>(maximumStep));
    };
    result.counts.x =
        axis(errorX, yaw, variation.x, vx_, fx_, caps ? static_cast<unsigned>(caps->x) : policy.maximumStepCounts);
    result.counts.y =
        axis(errorY, pitch, variation.y, vy_, fy_, caps ? static_cast<unsigned>(caps->y) : policy.maximumStepCounts);
    return result;
}
} // namespace mousebox::control::movement
