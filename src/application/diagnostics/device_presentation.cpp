#include "device_presentation.hpp"
#include "application/storage/json_store.hpp"
#include "render/directx_renderer.hpp"
#include <button_wire.h>

namespace mousebox::application
{
namespace
{
Json indicator(const char *phase, std::string message)
{
    return {{"phase", phase}, {"message", std::move(message)}};
}
Json serialIndicator(const Json &ports, bool checked, const std::string &error)
{
    if (!error.empty())
        return indicator("error", "串口检测失败 · " + error);
    if (!checked)
        return indicator("connecting", "正在检测串口");
    if (ports.empty())
        return indicator("closed", "未检测到 USB 转 TTL 设备");
    std::string names;
    for (const auto &port : ports)
    {
        if (!names.empty())
            names += " / ";
        names += port.at("port").get<std::string>();
    }
    return indicator(ports.size() > 1 ? "degraded" : "ready",
                     (ports.size() > 1 ? "检测到多个 USB 转 TTL 串口 · " : "已接入 · ") + names);
}
Json boxIndicator(const DeviceSnapshot &s, std::uint64_t now)
{
    if (!s.boxError.empty())
        return indicator("error", "连接失败 · " + s.boxError);
    if (s.boxConnecting)
        return indicator("connecting", "正在连接盒子");
    if (!s.boxReady)
        return indicator("closed", "未连接");
    const bool feedbackCurrent = s.boxFeedbackMs && now >= s.boxFeedbackMs && now - s.boxFeedbackMs <= 150;
    if (!s.triggerAvailable || !feedbackCurrent)
    {
        if (s.boxOpenedMs && now >= s.boxOpenedMs && now - s.boxOpenedMs < 2000)
            return indicator("connecting", "等待盒子有效回传");
        const auto message = !feedbackCurrent                                  ? "盒子回传超时或缺失"
                             : !(s.boxFeedbackFlags & mousebox_buttons::Ready) ? "盒子回传正常，接收器或USB输出未就绪"
                             : !(s.boxFeedbackFlags & mousebox_buttons::Known) ? "盒子回传正常，等待鼠标按钮状态"
                                                                               : "盒子回传正常，侧键状态暂不可用";
        auto result = indicator("degraded", message);
        // Receiving valid packets with not-ready flags is not a broken serial link.
        result["retryable"] = !feedbackCurrent;
        return result;
    }
    return indicator("ready", "盒子已连接，回传正常");
}
} // namespace

Json describeDeviceConnections(const DeviceSnapshot &s, const Json &ports, bool checked, const std::string &error,
                               std::uint64_t now)
{
    return {{"serial", serialIndicator(ports, checked, error)}, {"box", boxIndicator(s, now)}};
}

Json describeDevices(const OutputObservation &output, std::uint64_t now, const DevicePresentation &context)
{
    const auto &s = output.device;
    const auto &gpu = context.renderer;
    return {
        {"connections", describeDeviceConnections(s, context.ports, context.portsChecked, context.portsError, now)},
        {"boxReady", s.boxReady},
        {"testArmed", s.testArmed},
        {"button", !s.boxReady                               ? "盒子未连接"
                   : !s.triggerAvailable                     ? "等待侧键回传"
                   : (s.mouseButtons & output.triggerButton) ? "按下"
                                                             : "松开"},
        {"boxStatus", s.boxStatus},
        {"boxFeedbackAgeMs", s.boxFeedbackMs && now >= s.boxFeedbackMs ? Json(now - s.boxFeedbackMs) : Json{}},
        {"boxFeedbackSequence", s.boxFeedbackMs ? Json(s.boxFeedbackSequence) : Json{}},
        {"boxReceiverReady", s.boxFeedbackMs ? Json(bool(s.boxFeedbackFlags & mousebox_buttons::Ready)) : Json{}},
        {"boxButtonsKnown", s.boxFeedbackMs ? Json(bool(s.boxFeedbackFlags & mousebox_buttons::Known)) : Json{}},
        {"deviceBaudRate", s.deviceCapabilitiesKnown ? Json(s.deviceBaudRate) : Json{}},
        {"deviceMaximumStepCounts", s.deviceCapabilitiesKnown ? Json(s.motionLimits.maximumStepCounts) : Json{}},
        {"deviceMinimumIntervalMs", s.deviceCapabilitiesKnown ? Json(s.motionLimits.minimumIntervalMs) : Json{}},
        {"deviceMaximumCountsPerSecond",
         s.deviceCapabilitiesKnown ? Json(s.motionLimits.maximumCountsPerSecond()) : Json{}},
        {"deviceCapabilitiesKnown", s.deviceCapabilitiesKnown},
        {"renderer", !gpu.error.empty() ? "绘制暂不可用"
                     : !gpu.initialized ? "绘制准备中"
                     : gpu.hardware     ? "显卡绘制 · 数据到达刷新"
                                        : "兼容绘制"},
        {"adapter", storage::utf8(gpu.adapter)},
        {"adapterId", gpu.adapterId},
        {"adapterReady", gpu.initialized},
        {"adapterNotice", gpu.adapterFallback == render::AdapterFallback::None ? "" : "所选显卡不可用，已使用回退设备"},
        {"graphicsError", context.graphicsError}};
}
} // namespace mousebox::application
