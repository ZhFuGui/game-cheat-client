#pragma once
#include <stdint.h>

namespace mousebox_firmware
{
template <int16_t Step, uint16_t Interval, uint16_t Budget> struct MotionLimits
{
    static_assert(Step > 0 && Interval > 0 && Interval <= 50 && Budget > 0, "Invalid motion limits");
    enum
    {
        MaxStepCounts = Step,
        MinIntervalMs = Interval,
        MaxBudget = Budget
    };
};
struct DeviceCapabilities
{
    uint32_t baudRate;
    uint16_t minIntervalMs, maxBudget;
    int16_t maxStepCounts;
    uint8_t legacyFlags;
};
} // namespace mousebox_firmware
