#pragma once
#include <algorithm>

namespace mousebox::control
{
// Negotiated actuator limits; disconnected presentation uses the supported v5 baseline.
struct DeviceMotionLimits
{
    unsigned maximumStepCounts = 16;
    unsigned minimumIntervalMs = 5;
    unsigned maximumBudget = 64;
    bool operator==(const DeviceMotionLimits &) const = default;
    bool valid() const noexcept
    {
        // Signed 16-bit axes and an unsigned 16-bit XY budget are wire-format limits,
        // independent of the limits advertised by any particular firmware generation.
        return maximumStepCounts > 0 && maximumStepCounts <= 32767 && minimumIntervalMs >= 2 &&
               minimumIntervalMs <= 30 && maximumBudget >= 2 && maximumBudget <= 65535;
    }
    double maximumCountsPerSecond() const noexcept
    {
        return minimumIntervalMs ? 1000.0 * std::min(maximumStepCounts, maximumBudget / 2) / minimumIntervalMs : 0;
    }
};
} // namespace mousebox::control
