#pragma once
#include <button_wire.h>
#include <cstdint>
#include "control/device_motion.hpp"
namespace mousebox::control
{
struct ButtonObservation
{
    mousebox_buttons::State state{};
    std::uint64_t observedMs = 0;
    bool received = false;
    bool fresh(std::uint64_t now) const noexcept
    {
        return received && (state.flags & 3) == 3 && now >= observedMs && now - observedMs <= 150;
    }
    unsigned maximumStepCounts() const noexcept
    {
        if (received && (state.flags & mousebox_buttons::HighRateMotion))
            return 100;
        return received && (state.flags & mousebox_buttons::ExtendedMotion) ? 16 : 4;
    }
    unsigned minimumIntervalMs() const noexcept
    {
        return received && (state.flags & mousebox_buttons::HighRateMotion) ? 2 : 5;
    }
    DeviceMotionLimits motionLimits() const noexcept
    {
        return {maximumStepCounts(), minimumIntervalMs(),
                received && (state.flags & mousebox_buttons::HighRateMotion) ? 200u : 64u};
    }
    bool supportsStep(unsigned counts, std::uint64_t now) const noexcept
    {
        return fresh(now) && counts <= maximumStepCounts();
    }
    bool supportsMotion(unsigned counts, unsigned intervalMs, std::uint64_t now) const noexcept
    {
        return supportsStep(counts, now) && intervalMs >= minimumIntervalMs();
    }
    bool down(std::uint16_t mask) const noexcept
    {
        return (state.buttons & mask) != 0;
    }
    unsigned releases(std::uint16_t mask) const noexcept
    {
        return mask == mousebox_buttons::Back ? state.releases & 255 : state.releases >> 8;
    }
};
class ButtonFeedback
{
    mousebox_buttons::Parser parser_;
    ButtonObservation observation_;

  public:
    void feed(std::uint8_t byte, std::uint64_t now)
    {
        mousebox_buttons::State s{};
        if (!parser_.feed(byte, s))
            return;
        const auto delta = s.sequence - observation_.state.sequence;
        if (observation_.received && (!delta || delta >= 0x80000000u))
            return;
        observation_ = {s, now, true};
    }
    ButtonObservation observation() const noexcept
    {
        return observation_;
    }
};
} // namespace mousebox::control
