#include "backend_data.hpp"
#include "status_text.hpp"
#include "application/output/output_loop.hpp"
#include "platform/windows/input.hpp"

namespace mousebox::application
{
class Backend::DeviceClient final : public OutputClient
{
    Backend &backend_;
    MotionConfiguration next_, configuration_;
    std::uint64_t revision_ = 0;
    control::LiveController live_;
    unsigned previousRelease_ = 0;
    bool buttonsSeen_ = false;

  public:
    explicit DeviceClient(Backend &backend) : backend_(backend) {}
    OutputSelection select() override
    {
        const auto mode = backend_.outputMode();
        auto motion = backend_.motionConfiguration();
        if (motion != next_)
        {
            next_ = std::move(motion);
            ++revision_;
        }
        const auto activation = next_.externalProgram ? next_.externalProgram->activation() : std::nullopt;
        return {mode, next_.speed, next_.triggerButton, revision_, next_.externalProgram, activation};
    }
    bool current() const override
    {
        return backend_.outputMode() == control::OutputMode::AutomaticAim &&
               backend_.motionConfiguration() == configuration_;
    }
    std::uint64_t generation() const override
    {
        const std::lock_guard lock(backend_.data_->mutex);
        return backend_.data_->snapshot.generation;
    }
    void buttons(const control::ButtonObservation &observation, std::uint64_t now,
                 control::DeviceMotionLimits limits) override
    {
        backend_.buttonState(observation, now, limits);
    }
    void transitioned(control::DeviceMotionLimits limits) override
    {
        configuration_ = next_;
        buttonsSeen_ = false;
        if (configuration_.externalProgram)
            return;
        auto policy = control::speedPolicy(configuration_.speed, limits);
        policy.trajectory = configuration_.trajectory;
        live_ = control::LiveController(policy, configuration_.aimPart, configuration_.maximumAimDistanceMeters,
                                        configuration_.targetPriority, configuration_.weaponAim);
        live_.setHumanModel(configuration_.model, configuration_.dpi, configuration_.variation);
        if (control::CurrentBallisticsAssumptions.liveBallisticsIntegrated)
            live_.setBallistics(configuration_.ballisticCatalog, configuration_.zeroingDistanceMeters,
                                configuration_.gravityMetersPerSecondSquared, configuration_.livingLocalSkewMs);
    }
    void motion(bool down, const ports::ProgramStep &step, std::optional<std::uint64_t> generation) override
    {
        backend_.outputState(down, step.active, step.releaseRequired, step.motion.event, {}, false, {}, generation);
    }
    bool testArmed() const override
    {
        return backend_.testArmed_.load();
    }
    bool measurementPending() const override
    {
        return backend_.calibrateRequested_.load();
    }
    void measure(ports::OutputLink &link, std::stop_token stop) override
    {
        backend_.calibrate(link, stop);
    }
    void legacyTick(ports::OutputLink &link, const control::ButtonObservation &buttons,
                    control::DeviceMotionLimits limits, const std::function<std::uint32_t()> &nextSession,
                    std::stop_token stop) override;
};

void Backend::DeviceClient::legacyTick(ports::OutputLink &link, const control::ButtonObservation &buttons,
                                       control::DeviceMotionLimits limits,
                                       const std::function<std::uint32_t()> &nextSession, std::stop_token stop)
{
    const auto s = backend_.observeSnapshot();
    const auto controlNow = monotonicMilliseconds();
    const auto policy = control::speedPolicy(configuration_.speed, limits);
    const bool triggerValid = buttons.supportsMotion(policy.maximumStepCounts, policy.intervalMs, controlNow),
               down = triggerValid && buttons.down(configuration_.triggerButton);
    control::ObservedFrames frames{s.living, s.view, s.livingObservedMs, s.viewObservedMs, s.readerRunning};
    const unsigned released = buttons.releases(configuration_.triggerButton);
    if (triggerValid && buttonsSeen_ && released != previousRelease_)
    {
        // A released-and-repressed edge can be hidden in a USB/serial batch; cancel the old hold first.
        live_.step(false, s.generation, controlNow, frames, s.calibration, s.device.boxReady, s.sourceSkewMs);
        link.stop(0);
    }
    previousRelease_ = released;
    buttonsSeen_ = triggerValid;
    const auto result = live_.step(triggerValid ? down : true, s.generation, controlNow, frames, s.calibration,
                                   s.device.boxReady && triggerValid, s.sourceSkewMs);
    if (result.begin)
        backend_.log(result.event == control::MotionEvent::Resumed ? "锁定人物数据恢复，保持侧键继续跟随同一人物。"
                                                                   : "鼠标侧键平滑修正开始：" + calibrationText(s));
    if (result.event == control::MotionEvent::RejectedPress)
        backend_.log("侧键未启动：" +
                     (triggerValid ? blockText(live_.diagnostic(), s) : "等待固件/回传线提供新鲜侧键状态") +
                     "；先松开侧键再按。");
    if (result.stop)
    {
        link.stop(0);
        auto detail = motionEventText(result.event);
        if (live_.diagnostic().block != control::LiveBlock::Ready)
            detail += " · " + blockText(live_.diagnostic(), s);
        backend_.log("鼠标侧键平滑修正停止：" + detail + "。");
    }
    if (result.dx || result.dy)
    {
        const auto currentButtons = link.buttons();
        if (!current() || link.motionLimits() != limits ||
            !currentButtons.supportsMotion(policy.maximumStepCounts, policy.intervalMs, monotonicMilliseconds()) ||
            !currentButtons.down(configuration_.triggerButton) ||
            currentButtons.releases(configuration_.triggerButton) != released || stop.stop_requested())
        {
            live_.rejectStep(result);
            link.stop(0);
        }
        else if (!backend_.outputObservationCurrent(s.observationRevision))
            live_.rejectStep(result);
        else if (!link.finiteCorrection(nextSession(), result.dx, result.dy))
            live_.rejectStep(result);
    }
    backend_.outputState(down, live_.active(), live_.releaseRequired(), live_.stoppedBy(), live_.heldTarget(),
                         live_.suspended(), live_.feedback(), s.generation);
}

std::unique_ptr<OutputClient> Backend::makeDeviceClient()
{
    return std::make_unique<DeviceClient>(*this);
}
} // namespace mousebox::application
