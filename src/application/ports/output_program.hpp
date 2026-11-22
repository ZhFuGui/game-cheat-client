#pragma once
#include "control/device_motion.hpp"
#include "control/motion_step.hpp"
#include "control/speed_policy.hpp"
#include "control/activation.hpp"
#include <cstdint>
#include <memory>
#include <optional>

namespace mousebox::application::ports
{
struct OutputTick
{
    std::uint64_t nowMs = 0, generation = 0;
    bool buttonFresh = false, down = false;
    control::DeviceMotionLimits limits;
    // Existing device feedback counts real releases even when a level/packet
    // was missed. Absent while feedback is stale, never inferred from silence.
    std::optional<unsigned> releaseCounter;
};
struct ProgramStep
{
    control::MotionStep motion;
    std::uint64_t sourceRevision = 0;
    bool active = false, releaseRequired = false;
};
struct ButtonPulseTick
{
    std::uint64_t nowMs = 0, generation = 0;
    bool fresh = false, supported = false, physicalLeftDown = false;
};
struct ButtonPulseStep
{
    std::uint64_t sourceRevision = 0;
    unsigned holdMs = 20;
};
// Game-owned calculations receive current device facts, never serial transport.
class OutputProgram
{
  public:
    virtual ~OutputProgram() = default;
    // Called under the device owner's mutex: atomic/non-blocking observation;
    // implementations must not call the owner or acquire a runtime state lock.
    virtual bool requested() const noexcept = 0;
    // Absent preserves the existing configured physical side-button contract.
    // A custom activation is polled/rechecked by the one native device owner.
    virtual std::optional<control::ActivationBinding> activation() const noexcept
    {
        return {};
    }
    virtual ProgramStep tick(const OutputTick &) = 0;
    virtual bool current(std::uint64_t sourceRevision, std::uint64_t nowMs) const = 0;
    virtual void reject(const ProgramStep &) = 0;
    virtual void reset() = 0;
    // Independent finite button effects share the same actuator owner and lease.
    // They never reuse movement activation or manufacture physical feedback.
    virtual std::optional<ButtonPulseStep> buttonPulse(const ButtonPulseTick &)
    {
        return {};
    }
    virtual bool buttonPulseCurrent(const ButtonPulseStep &, std::uint64_t) const
    {
        return false;
    }
    virtual void buttonPulseAccepted(bool) {}
};
// Composition binds this port to the one global device owner. A game cannot
// open a second serial link, change baud rates, or route another game's data.
class OutputBinding
{
  public:
    virtual ~OutputBinding() = default;
    virtual void bind(std::shared_ptr<OutputProgram>, control::SpeedPreset, std::uint16_t trigger) = 0;
    virtual void eligible(bool) = 0;
    virtual void stop() = 0;
};
} // namespace mousebox::application::ports
