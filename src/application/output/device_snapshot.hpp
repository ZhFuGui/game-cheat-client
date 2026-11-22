#pragma once
#include "control/device_motion.hpp"
#include <cstdint>
#include <string>

namespace mousebox::application
{
struct DeviceSnapshot
{
    bool boxReady = false, boxConnecting = false, testArmed = false, triggerAvailable = false;
    std::string boxStatus = "未连接", boxError;
    std::uint64_t boxOpenedMs = 0, boxFeedbackMs = 0;
    std::uint32_t boxFeedbackSequence = 0;
    std::uint8_t boxFeedbackFlags = 0;
    control::DeviceMotionLimits motionLimits;
    unsigned deviceBaudRate = 0;
    bool deviceCapabilitiesKnown = false;
    std::uint16_t mouseButtons = 0;
};

struct OutputObservation
{
    DeviceSnapshot device;
    std::uint16_t triggerButton = 0;
    bool sessionEligible = false, aimArmed = false, calibrating = false, capturing = false;
};
} // namespace mousebox::application
