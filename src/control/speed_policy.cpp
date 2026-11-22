#include "control/speed_policy.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace mousebox::control
{
namespace
{
struct RelativeSpeed
{
    double fraction, gain, accelerationFactor, jerkFactor;
};
constexpr std::array<RelativeSpeed, 5> RelativeSpeeds{{{.1875, 20, 1.875, 93.75},
                                                       {.25, 28, 2.8125, 187.5},
                                                       {.5, 40, 7.5, 500},
                                                       {.75, 60, 15, 1000},
                                                       {1, 80, 22.5, 1562.5}}};
const RelativeSpeed &relativeSpeed(SpeedPreset preset)
{
    const auto index = static_cast<unsigned>(preset);
    if (index >= RelativeSpeeds.size())
        throw std::invalid_argument("Invalid speed preset");
    return RelativeSpeeds[index];
}
} // namespace
double speedFraction(SpeedPreset preset)
{
    return relativeSpeed(preset).fraction;
}
MotionPolicy speedPolicy(SpeedPreset preset, DeviceMotionLimits limits)
{
    const auto &p = relativeSpeed(preset);
    if (!limits.valid())
        throw std::invalid_argument("Invalid device motion limits");
    const auto maximum = limits.maximumCountsPerSecond();
    const auto speed = maximum * p.fraction;
    const auto step = std::clamp(static_cast<unsigned>(std::ceil(speed * limits.minimumIntervalMs / 1000)), 1u,
                                 std::min(limits.maximumStepCounts, limits.maximumBudget / 2));
    return {p.gain,
            speed,
            maximum * p.accelerationFactor,
            .05,
            0,
            0,
            limits.minimumIntervalMs,
            step,
            maximum * p.jerkFactor};
}
SpeedPreset storedSpeedPreset(int value, int schema, bool present) noexcept
{
    if (!present)
        return SpeedPreset::Normal;
    if (schema < 2)
        return value == 2 ? SpeedPreset::Slow : SpeedPreset::Slowest;
    return value >= 0 && value <= 4 ? static_cast<SpeedPreset>(value) : SpeedPreset::Normal;
}
} // namespace mousebox::control
