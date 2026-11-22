#pragma once
#include "protocol/control_codec.hpp"
#include <array>
#include <cstdlib>
#include <stdexcept>
#include "control/device_motion.hpp"
namespace mousebox
{
inline void validateDeviceCommand(const ControlCommand &command, control::DeviceMotionLimits limits)
{
    if (!limits.valid())
        throw std::invalid_argument("Invalid negotiated device capability");
    if (command.type == mousebox_wire::Start && command.budget > limits.maximumBudget)
        throw std::invalid_argument("Session budget exceeds negotiated device capability");
    if (command.type == mousebox_wire::Motion &&
        (std::abs(static_cast<int>(command.dx)) > static_cast<int>(limits.maximumStepCounts) ||
         std::abs(static_cast<int>(command.dy)) > static_cast<int>(limits.maximumStepCounts) ||
         command.intervalMs < limits.minimumIntervalMs))
        throw std::invalid_argument("Motion exceeds negotiated device capability");
}
inline std::array<ControlCommand, 2> oneShot(std::uint32_t session, int x, int y, control::DeviceMotionLimits limits)
{
    const auto maximum = static_cast<int>(limits.maximumStepCounts);
    if (!limits.valid() || !session || x < -maximum || x > maximum || y < -maximum || y > maximum || (!x && !y) ||
        std::abs(x) + std::abs(y) > static_cast<int>(limits.maximumBudget))
        throw std::invalid_argument("Invalid one-shot displacement");
    const auto cost = std::abs(x) + std::abs(y);
    // An unclipped USB submission consumes this entire budget. Saturation may leave
    // a bounded remainder, still governed by the same 20ms lease and total XY budget.
    return {{{mousebox_wire::Start, session, 1, 0, 0, 0, 0, static_cast<std::uint16_t>(cost)},
             {mousebox_wire::Motion, session, 2, static_cast<std::int16_t>(x), static_cast<std::int16_t>(y),
              static_cast<std::uint16_t>(limits.minimumIntervalMs), 20, 0}}};
}
} // namespace mousebox
