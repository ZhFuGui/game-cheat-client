#include "runtime_service_impl.hpp"
#include "runtime_service_util.hpp"
#include "application/diagnostics/device_presentation.hpp"
#include "games/pubg/diagnostics/aim_algorithm.hpp"
#include "games/pubg/diagnostics/runtime_state.hpp"

namespace mousebox::application
{
using namespace runtime_detail;

void RuntimeService::setRenderStats(render::RenderStats value)
{
    const std::lock_guard lock(impl_->mutex);
    impl_->renderStats = std::move(value);
}

render::EspOptions RuntimeService::espOptions() const
{
    const std::lock_guard lock(impl_->mutex);
    const auto &v = impl_->settings["espValues"];
    render::EspOptions o;
    const auto marker = v["marker"].get<std::string>(), aspect = v["aspect"].get<std::string>();
    o.marker = marker == "point"       ? render::EspMarker::Point
               : marker == "estimated" ? render::EspMarker::Estimated
                                       : render::EspMarker::Corners;
    o.aspect = aspect == "16:10"      ? render::EspAspect::StandardWide
               : aspect == "21:9"     ? render::EspAspect::UltraWide
               : aspect == "4:3"      ? render::EspAspect::Classic
               : aspect == "viewport" ? render::EspAspect::Viewport
                                      : render::EspAspect::Wide;
    o.fovAxis = v["fovAxis"] == "vertical" ? render::EspFovAxis::Vertical : render::EspFovAxis::Horizontal;
    o.names = v["names"].get<bool>();
    o.distance = v["distance"].get<bool>();
    o.bones = v["bones"].get<bool>();
    o.tracers = v["tracers"].get<bool>();
    o.crosshair = v["crosshair"].get<bool>();
    o.aimRange = v["aimRange"].get<bool>();
    o.targetHighlight = v["targetHighlight"].get<bool>();
    o.teammates = v["teammates"].get<bool>();
    o.opponents = v["opponents"].get<bool>();
    o.bots = v["bots"].get<bool>();
    o.self = v["self"].get<bool>();
    o.estimatedHeightMeters = v["estimatedHeightMeters"].get<double>();
    o.maximumDistanceMeters = v["maximumDistanceMeters"].get<double>();
    return o;
}

Json RuntimeService::snapshot(bool includeDiagnostics)
{
    poll();
    auto &d = *impl_;
    const std::lock_guard lock(d.mutex);
    const auto now = monotonicMilliseconds();
    Json state, algorithm = Json::object(), sections = Json::array();
    OutputObservation output;
    if (auto app = d.gameModule->application())
    {
        output = d.backend.inspectOutput();
        auto game = app->snapshot(includeDiagnostics || d.algorithmSubscribed);
        state = std::move(game.at("state"));
        require(state.is_object() && state.contains("connections") && state.at("connections").is_object() &&
                    state.at("connections").contains("reader"),
                "游戏应用没有提供独立运行状态");
        state["sessionEligible"] = output.sessionEligible;
        state["aimArmed"] = state.value("aimArmed", false) && output.aimArmed;
        state["canAim"] = state.value("canAim", false) && output.device.boxReady && output.device.triggerAvailable;
        state["canCalibrate"] = state.value("canCalibrate", false) && output.device.boxReady &&
                                output.device.triggerAvailable && !output.capturing && !output.device.testArmed &&
                                !output.calibrating && !d.ioBusy;
        state["espDrawFramesPerSecond"] = d.renderStats.drawFramesPerSecond;
        state["espVisible"] = "可见 " + std::to_string(d.renderStats.esp.counts.visible) + " / 人物 " +
                              std::to_string(d.renderStats.esp.counts.input);
        if (state.contains("readerMetrics"))
        {
            const bool gpu = d.renderStats.initialized && d.renderStats.gpuCompletionAvailable;
            state["readerMetrics"]["gpuCompletionAvailable"] = gpu;
            if (!gpu)
                for (auto &channel : state["readerMetrics"]["channels"])
                {
                    channel["averageMs"] = nullptr;
                    channel["uncertaintyMs"] = nullptr;
                    channel["samples"] = 0;
                    if (channel["status"] == "ready")
                        channel["status"] = "waiting-draw";
                }
        }
        algorithm = game.value("algorithm", Json::object());
        sections = game.value("algorithmSections", Json::array());
    }
    else
    {
        auto s = d.backend.inspectSnapshot();
        s.readerMetrics = d.backend.readerMetricsSnapshot(now);
        output = {s.device,   s.motion.triggerButton, d.backend.sessionEligible(),
                  s.aimArmed, s.calibrating,          s.capturing};
        control::LiveController diagnostic(control::speedPolicy(s.speed, s.device.motionLimits), s.motion.aimPart,
                                           s.motion.maximumAimDistanceMeters, s.motion.targetPriority,
                                           s.motion.weaponAim);
        if (control::CurrentBallisticsAssumptions.liveBallisticsIntegrated)
            diagnostic.setBallistics(s.motion.ballisticCatalog, s.motion.zeroingDistanceMeters,
                                     s.motion.gravityMetersPerSecondSquared, s.motion.livingLocalSkewMs);
        const auto report =
            diagnostic.inspect(now, {s.living, s.view, s.livingObservedMs, s.viewObservedMs, s.readerRunning},
                               s.calibration, s.device.boxReady, s.sourceSkewMs, s.heldTarget);
        state = describeRuntimeState(s, report, now, {output.sessionEligible, d.renderStats, d.ballisticsError});
        if (includeDiagnostics || d.algorithmSubscribed)
        {
            auto a = describeAimAlgorithm(s, report, now);
            algorithm = std::move(a.values);
            sections = std::move(a.sections);
        }
    }
    auto devices =
        describeDevices(output, now, {d.ports, d.portsChecked, d.portsError, d.renderStats, d.graphicsError});
    for (auto it = devices.begin(); it != devices.end(); ++it)
        if (it.key() != "connections")
            state[it.key()] = it.value();
    state["connections"].update(devices.at("connections"));

    const bool completion = d.renderStats.initialized && d.renderStats.gpuCompletionAvailable;
    const auto &timing = d.renderStats.timing;
    state["espTiming"] = {{"windowMs", 2000},
                          {"completionAvailable", completion},
                          {"processingMs", completion && timing.processingMs ? Json(*timing.processingMs) : Json{}},
                          {"drawingMs", completion && timing.drawingMs ? Json(*timing.drawingMs) : Json{}},
                          {"samples", completion ? timing.samples : 0}};
    return {{"application", d.workspaceStore.applicationSettings()},
            {"devices", std::move(devices)},
            {"workspace", d.workspaceCatalog()},
            {"mouseReports", d.reportState()},
            {"speedPresets", speedPresets(output.device.motionLimits)},
            {"values", d.values()},
            {"espValues", d.settings["espValues"]},
            {"state", std::move(state)},
            {"learning", d.learning()},
            {"ports", d.ports},
            {"graphicsAdapters", d.graphics},
            {"algorithm", std::move(algorithm)},
            {"algorithmSections", std::move(sections)}};
}
} // namespace mousebox::application
