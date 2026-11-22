#include "backend_data.hpp"
#include "adapters/outputs/mousebox/serial_session.hpp"
#include "application/output/output_loop.hpp"
#include "platform/windows/input.hpp"
#include <Windows.h>
#include "platform/windows/ports.hpp"

namespace mousebox::application
{
std::vector<Port> Backend::ports() const
{
    std::vector<Port> result;
    for (const auto &p : serialDevices())
        if (p.ttl)
            result.push_back({p.port, p.name});
    return result;
}

void Backend::disconnectBox()
{
    testArmed_ = false;
    calibrateRequested_ = false;
    device_->disconnect();
    const std::lock_guard lock(data_->mutex);
    data_->snapshot.device.boxReady = false;
    data_->snapshot.device.triggerAvailable = false;
    data_->snapshot.device.mouseButtons = 0;
    data_->snapshot.device.boxStatus = "未连接";
    data_->snapshot.device.boxConnecting = false;
    data_->snapshot.device.boxOpenedMs = data_->snapshot.device.boxFeedbackMs = 0;
    data_->snapshot.device.boxFeedbackSequence = data_->snapshot.device.boxFeedbackFlags = 0;
    data_->snapshot.device.boxError.clear();
    data_->snapshot.device.motionLimits = {};
    data_->snapshot.device.deviceBaudRate = 0;
    data_->snapshot.device.deviceCapabilitiesKnown = false;
    data_->snapshot.heldTarget.reset();
    data_->snapshot.suspended = false;
    data_->snapshot.controlActive = false;
    data_->snapshot.motionFeedback.reset();
}

void Backend::buttonState(const control::ButtonObservation &buttons, std::uint64_t now,
                          control::DeviceMotionLimits limits)
{
    const std::lock_guard lock(data_->mutex);
    data_->snapshot.device.triggerAvailable = buttons.fresh(now);
    data_->snapshot.device.boxFeedbackMs = buttons.received ? buttons.observedMs : 0;
    data_->snapshot.device.boxFeedbackSequence = buttons.received ? buttons.state.sequence : 0;
    data_->snapshot.device.boxFeedbackFlags = buttons.received ? buttons.state.flags : 0;
    data_->snapshot.device.mouseButtons = buttons.fresh(now) ? buttons.state.buttons : 0;
    if (buttons.received)
    {
        if (data_->snapshot.device.motionLimits != limits)
            ++data_->snapshot.generation;
        data_->snapshot.device.motionLimits = limits;
    }
}

void Backend::connectBox(const std::wstring &requested)
{
    disconnectBox();
    {
        const std::lock_guard lock(data_->mutex);
        data_->snapshot.device.boxConnecting = true;
    }
    device_->connect(
        requested, [this](unsigned baud, control::DeviceMotionLimits limits) { boxOpened(baud, limits); },
        [this](ports::OutputLink &link, std::stop_token stop) {
            auto client = makeDeviceClient();
            OutputLoop loop(link, *client, {monotonicMilliseconds, keyDown, VK_F8}, randomSession(), stop);
            PollTimer timer;
            while (!stop.stop_requested())
                timer.wait(loop.tick());
        },
        [this](const std::string &error) { boxFailed(error); });
}

void Backend::boxOpened(unsigned baud, control::DeviceMotionLimits limits)
{
    {
        const std::lock_guard lock(data_->mutex);
        auto &s = data_->snapshot;
        s.device.boxReady = true;
        s.device.boxConnecting = false;
        s.device.boxOpenedMs = monotonicMilliseconds();
        s.device.boxStatus = "已连接 · 侧键跟随自动待命";
        s.device.motionLimits = limits;
        s.device.deviceBaudRate = baud;
        s.device.deviceCapabilitiesKnown = true;
    }
    log("鼠标盒子已连接 · " + std::to_string(baud) + " baud · 单轴 " + std::to_string(limits.maximumStepCounts) +
        " counts / " + std::to_string(limits.minimumIntervalMs) + " ms。");
}

void Backend::boxFailed(const std::string &error)
{
    testArmed_ = false;
    {
        const std::lock_guard lock(data_->mutex);
        auto &s = data_->snapshot;
        s.device.boxReady = false;
        s.device.triggerAvailable = false;
        s.device.mouseButtons = 0;
        s.device.boxStatus = "连接失败或已断开";
        s.device.motionLimits = {};
        s.device.deviceBaudRate = 0;
        s.device.deviceCapabilitiesKnown = false;
        s.device.boxConnecting = false;
        s.device.boxError = error;
    }
    outputState(false, false, false, control::MotionEvent::EligibilityLost);
    log("鼠标盒子：" + error);
}
} // namespace mousebox::application
