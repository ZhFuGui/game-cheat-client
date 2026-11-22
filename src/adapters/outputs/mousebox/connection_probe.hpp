#pragma once
#include "control/button_feedback.hpp"
#include "protocol/control_codec.hpp"
#include <array>
#include <algorithm>
#include <stdexcept>

namespace mousebox
{
inline control::DeviceMotionLimits effectiveMotionLimits(control::DeviceMotionLimits limits, unsigned baudRate)
{
    if (!baudRate)
        throw std::invalid_argument("Missing negotiated baud rate");
    // 8N1 and a START+MOTION pair. Never schedule faster than the wire can serialize.
    constexpr unsigned pairBitsMs = 2 * mousebox_wire::FrameSize * 10 * 1000;
    const auto wireInterval = (pairBitsMs + baudRate - 1) / baudRate;
    limits.minimumIntervalMs = std::max(limits.minimumIntervalMs, wireInterval);
    return limits;
}

class ConnectionProbe
{
    unsigned index_ = 0, confirmations_ = 0;
    std::uint64_t startedMs_ = 0, firstConfirmationMs_ = 0;
    control::ButtonFeedback feedback_;

  public:
    // Known installed firmware rates. Future rates extend this adapter, not speed presets.
    static constexpr std::array<unsigned, 2> BaudRates{115200, 500000};
    static constexpr unsigned TimeoutMs = 300;
    explicit ConnectionProbe(std::uint64_t now) : startedMs_(now) {}
    unsigned baudRate() const noexcept
    {
        return BaudRates[index_];
    }
    bool expired(std::uint64_t now) const noexcept
    {
        return now >= startedMs_ && now - startedMs_ >= TimeoutMs;
    }
    bool next(std::uint64_t now)
    {
        if (index_ + 1 == BaudRates.size())
            return false;
        ++index_;
        startedMs_ = now;
        feedback_ = {};
        confirmations_ = 0;
        firstConfirmationMs_ = 0;
        return true;
    }
    void feed(std::uint8_t byte, std::uint64_t now)
    {
        const auto before = feedback_.observation();
        feedback_.feed(byte, now);
        const auto after = feedback_.observation();
        if (after.received && (!before.received || after.state.sequence != before.state.sequence))
        {
            const bool same = before.received && after.motionLimits() == before.motionLimits();
            confirmations_ = same ? confirmations_ + 1 : 1;
            if (!same)
                firstConfirmationMs_ = now;
        }
    }
    bool confirmed() const noexcept
    {
        const auto observed = feedback_.observation().observedMs;
        return confirmations_ >= 2 && observed >= firstConfirmationMs_ && observed - firstConfirmationMs_ >= 10;
    }
    const control::ButtonFeedback &feedback() const noexcept
    {
        return feedback_;
    }
};
} // namespace mousebox
