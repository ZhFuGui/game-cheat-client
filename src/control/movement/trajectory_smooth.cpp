#include "trajectory_smooth.hpp"
#include "path_input.hpp"
#include <algorithm>
#include <cmath>
#include <ruckig/ruckig.hpp>
namespace mousebox::control::movement
{
std::optional<Counts> TrajectorySmooth::step(const TrajectoryInput &in, HumanMotionRuntime &human)
{
    rollback_.reset();
    if (in.movement.policy.trajectory.kind != TrajectoryKind::Bezier)
        return stepLinear(in, human);
    rollback_ = State{vx_, vy_, fx_, fy_, ax_, ay_, path_};
    auto scalar = in;
    scalar.movement = pathInput(in.movement, path_);
    const auto direction = path_.chordDirection();
    scalar.targetVelocity = {in.targetVelocity.x * direction.x + in.targetVelocity.y * direction.y, 0};
    scalar.deadZoneDegrees = .5;
    // Learned limits continue to apply; lateral variation belongs to geometry.
    const auto counts = stepLinear(scalar, human);
    if (!counts)
        return {};
    Result result;
    result.counts = *counts;
    return pathResult(result, in.movement, path_).counts;
}
void TrajectorySmooth::reject() noexcept
{
    if (rollback_)
    {
        vx_ = rollback_->vx;
        vy_ = rollback_->vy;
        fx_ = rollback_->fx;
        fy_ = rollback_->fy;
        ax_ = rollback_->ax;
        ay_ = rollback_->ay;
        path_ = rollback_->path;
        rollback_.reset();
    }
}
std::optional<Counts> TrajectorySmooth::stepLinear(const TrajectoryInput &in, HumanMotionRuntime &human)
{
    const auto &policy = in.movement.policy;
    const auto dt = in.movement.dt;
    const auto now = in.movement.now;
    const auto deadZone = in.deadZoneDegrees;
    const auto estimated = in.geometryAvailable;
    const auto targetVx = in.targetVelocity.x, targetVy = in.targetVelocity.y;
    const auto angularDistance = std::hypot(in.movement.errorDegrees.x, in.movement.errorDegrees.y);
    const auto limits = human.limits();
    const double speedLimit =
        human.enabled() ? std::min(policy.maximumCountsPerSecond, limits.speed) : policy.maximumCountsPerSecond;
    const double acceleration =
        human.enabled() ? std::min(policy.acceleration, limits.acceleration) : policy.acceleration;
    const double braking = human.enabled() ? std::min(policy.acceleration, limits.braking) : policy.acceleration;
    const auto variation =
        human.offset(in.movement.errorDegrees.x / in.movement.degreesPerCount.x,
                     in.movement.errorDegrees.y / in.movement.degreesPerCount.y, angularDistance, now);
    const auto axis = [&](double error, double scale, double offset, double &velocity, double &fraction) {
        if (std::abs(error) <= deadZone)
        {
            velocity = fraction = 0;
            return 0;
        }
        // Without geometry/camera feedback there is no pending-motion prediction.
        // Keep gain conservative at every error size: higher transport caps must not
        // turn a repeated delayed angle into repeated full-speed corrections.
        const double gain = estimated ? policy.gain : std::min(policy.gain, 3.0);
        const double distance = std::abs(error / scale), brakeLimit = std::sqrt(2 * braking * distance);
        const double limit = std::min(speedLimit, brakeLimit);
        const double requested = std::clamp((error / scale + offset) * gain, -limit, limit);
        if (velocity * requested < 0)
            velocity = fraction = 0;
        const double change = std::abs(requested) < std::abs(velocity) ? braking : acceleration;
        velocity += std::clamp(requested - velocity, -change * dt, change * dt);
        const double total = fraction + velocity * dt;
        const int allowance =
            static_cast<int>(std::min(static_cast<double>(policy.maximumStepCounts), std::floor(distance + 0.5)));
        const int count = std::clamp(static_cast<int>(std::trunc(total)), -allowance, allowance);
        fraction = std::abs(total) > allowance ? 0 : total - count;
        return count;
    };
    int dx = 0, dy = 0;
    if (in.tracking)
    {
        const double gain = std::min(policy.gain, in.trackingGain);
        const double servoAcceleration = 4 * std::min(policy.acceleration, 250000.0);
        const auto servo = [&](double error, double scale, double feed, double &velocity, double &fraction) {
            if (!feed && std::abs(error) <= deadZone)
            {
                velocity = fraction = 0;
                return 0;
            }
            const double remaining = error / scale;
            const double requested =
                std::clamp(feed + gain * remaining, -policy.maximumCountsPerSecond, policy.maximumCountsPerSecond);
            if (velocity * requested < 0)
                velocity = 0;
            velocity += std::clamp(requested - velocity, -servoAcceleration * dt, servoAcceleration * dt);
            const double total = fraction + velocity * dt;
            const int allowance =
                static_cast<int>(std::min(static_cast<double>(policy.maximumStepCounts),
                                          std::floor(std::abs(remaining) + std::abs(feed) * dt + 0.5)));
            const int count = std::clamp(static_cast<int>(std::trunc(total)), -allowance, allowance);
            fraction = std::abs(total) > allowance ? 0 : total - count;
            const bool following = feed && count * feed > 0 && std::abs(remaining) <= std::abs(feed) * dt + 0.5;
            if (count && count * remaining < 0 && !following)
            {
                velocity = fraction = 0;
                return 0;
            }
            return count;
        };
        dx = servo(in.movement.errorDegrees.x, in.movement.degreesPerCount.x, targetVx, vx_, fx_);
        dy = servo(in.movement.errorDegrees.y, in.movement.degreesPerCount.y, targetVy, vy_, fy_);
        ax_ = ay_ = 0;
    }
    else if (policy.maximumJerk && estimated)
    {
        // Use the published, local Ruckig state-to-state generator: short jerk-limited acceleration/braking,
        // not an exponential error-proportional crawl near the endpoint. Replan from current velocity/acceleration.
        ruckig::InputParameter<2> input;
        ruckig::Trajectory<2> trajectory;
        ruckig::Ruckig<2> generator;
        constexpr double unit = 1000; // Scale raw counts for the trajectory solver's numerical range.
        input.current_position = {0, 0};
        input.current_velocity = {vx_ / unit, vy_ / unit};
        input.current_acceleration = {ax_ / unit, ay_ / unit};
        input.target_position = {(in.movement.errorDegrees.x / in.movement.degreesPerCount.x + variation.x) / unit,
                                 (in.movement.errorDegrees.y / in.movement.degreesPerCount.y + variation.y) / unit};
        input.target_velocity = {std::clamp(targetVx, -speedLimit * 0.9, speedLimit * 0.9) / unit,
                                 std::clamp(targetVy, -speedLimit * 0.9, speedLimit * 0.9) / unit};
        input.max_velocity = {speedLimit / unit, speedLimit / unit};
        const double changeLimit = std::min(acceleration, braking) / unit;
        input.max_acceleration = {changeLimit, changeLimit};
        input.max_jerk = {policy.maximumJerk / unit, policy.maximumJerk / unit};
        input.synchronization = ruckig::Synchronization::Phase;
        const auto result = generator.calculate(input, trajectory);
        if (result < ruckig::Result::Working)
            return {};
        std::array<double, 2> position, velocity, change;
        trajectory.at_time(dt, position, velocity, change);
        for (unsigned i = 0; i < 2; ++i)
        {
            position[i] *= unit;
            velocity[i] *= unit;
            change[i] *= unit;
        }
        vx_ = velocity[0];
        vy_ = velocity[1];
        ax_ = change[0];
        ay_ = change[1];
        const auto quantize = [&](unsigned i, double error, double scale, double feed, double &fraction, double &v,
                                  double &a) {
            const double travel = std::abs(feed) * dt;
            const int allowance = static_cast<int>(std::min(static_cast<double>(policy.maximumStepCounts),
                                                            std::floor(std::abs(error / scale) + travel + 0.5)));
            const double total = fraction + position[i];
            const int count = std::clamp(static_cast<int>(std::trunc(total)), -allowance, allowance);
            fraction = std::abs(total) > allowance ? 0 : total - count;
            const bool following = feed && count * feed > 0 && std::abs(error / scale) <= travel + 0.5;
            if ((!feed && std::abs(error) <= deadZone) || (count && count * error / scale < 0 && !following))
            {
                fraction = v = a = 0;
                return 0;
            }
            return count;
        };
        dx = quantize(0, in.movement.errorDegrees.x, in.movement.degreesPerCount.x, targetVx, fx_, vx_, ax_);
        dy = quantize(1, in.movement.errorDegrees.y, in.movement.degreesPerCount.y, targetVy, fy_, vy_, ay_);
    }
    else
    {
        dx = axis(in.movement.errorDegrees.x, in.movement.degreesPerCount.x, variation.x, vx_, fx_);
        dy = axis(in.movement.errorDegrees.y, in.movement.degreesPerCount.y, variation.y, vy_, fy_);
    }
    return Counts{dx, dy};
}
} // namespace mousebox::control::movement
