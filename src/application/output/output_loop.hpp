#pragma once
#include "application/ports/output_link.hpp"
#include "application/ports/output_program.hpp"
#include "control/hold_plan.hpp"
#include "control/output_mode.hpp"
#include <functional>
#include <stop_token>

namespace mousebox::application
{
struct OutputSelection
{
    control::OutputMode mode = control::OutputMode::Idle;
    control::SpeedPreset speed = control::SpeedPreset::Normal;
    std::uint16_t trigger = mousebox_buttons::Back;
    std::uint64_t configuration = 0;
    std::shared_ptr<ports::OutputProgram> program;
    std::optional<control::ActivationBinding> activation;
    bool operator==(const OutputSelection &) const = default;
};

struct OutputEnvironment
{
    std::function<std::uint64_t()> now;
    std::function<bool(int)> keyDown;
    int testKey;
};

class OutputClient
{
  public:
    virtual ~OutputClient() = default;
    // Selection and the optional compatibility path are evaluated only on the
    // output worker. current() rechecks the live lease at submission time.
    virtual OutputSelection select() = 0;
    virtual bool current() const = 0;
    virtual std::uint64_t generation() const = 0;
    virtual void buttons(const control::ButtonObservation &, std::uint64_t now, control::DeviceMotionLimits) = 0;
    virtual void transitioned(control::DeviceMotionLimits) = 0;
    virtual void motion(bool down, const ports::ProgramStep &, std::optional<std::uint64_t> generation) = 0;
    virtual bool testArmed() const = 0;
    virtual bool measurementPending() const
    {
        return false;
    }
    virtual void measure(ports::OutputLink &, std::stop_token) {}
    virtual void legacyTick(ports::OutputLink &, const control::ButtonObservation &, control::DeviceMotionLimits,
                            const std::function<std::uint32_t()> &, std::stop_token)
    {
    }
};

// One loop per open link. The caller owns connection lifetime and sleeping.
class OutputLoop
{
    ports::OutputLink &link_;
    OutputClient &client_;
    OutputEnvironment environment_;
    std::stop_token stop_;
    HoldPlan test_{320};
    OutputSelection selection_;
    control::DeviceMotionLimits limits_;
    std::uint32_t session_, sequence_ = 0;

    std::uint32_t nextSession();
    void gate(bool enabled);
    void desktopTest(std::uint64_t now);
    void automatic(const control::ButtonObservation &);

  public:
    OutputLoop(ports::OutputLink &, OutputClient &, OutputEnvironment, std::uint32_t session, std::stop_token);
    unsigned tick();
};
} // namespace mousebox::application
