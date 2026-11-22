#pragma once
#include "control/device_motion.hpp"
#include "control/motion_policy.hpp"

namespace mousebox::control
{
enum class SpeedPreset
{
    Slowest,
    Slow,
    Normal,
    Fast,
    Fastest
};
double speedFraction(SpeedPreset);
MotionPolicy speedPolicy(SpeedPreset, DeviceMotionLimits limits = {});
SpeedPreset storedSpeedPreset(int value, int schema, bool present = true) noexcept;
} // namespace mousebox::control
