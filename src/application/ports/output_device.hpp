#pragma once
#include "control/device_motion.hpp"
#include <cstdint>

namespace mousebox::application::ports
{
// Actuator-only contract. No Reader frame, game angles, box wire command or
// firmware generation enters this interface. A rejected finite step is not queued.
class OutputDevice
{
  public:
    virtual ~OutputDevice() = default;
    virtual control::DeviceMotionLimits motionLimits() = 0;
    virtual bool finiteCorrection(std::uint32_t session, int dx, int dy) = 0;
    virtual void stop(std::uint32_t session) = 0;
    virtual bool supportsButtonPulses()
    {
        return false;
    }
    virtual bool finiteLeftClick(std::uint32_t, unsigned)
    {
        return false;
    }
};
} // namespace mousebox::application::ports
