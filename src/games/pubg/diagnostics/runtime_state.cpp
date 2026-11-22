#include "runtime_state.hpp"
#include "games/pubg/runtime/status_text.hpp"
#include "application/settings/settings.hpp"
#include "games/pubg/diagnostics/connection_status.hpp"
#include "games/pubg/pipeline/latency_prediction.hpp"
#include "domain/connection_policy.hpp"
#include "application/diagnostics/presentation_format.hpp"
#include "reader_metrics_presentation.hpp"

namespace mousebox::application
{
using presentation::nullable;
using presentation::number;

Json describeRuntimeState(const Snapshot &s, const control::LiveDiagnostic &report, std::uint64_t now,
                          const RuntimePresentation &context)
{
    const auto &gpu = context.renderer;
    std::string controlText;
    if (!context.sessionEligible)
        controlText = "运行会话未获资格，输出已停止";
    else if (s.calibrating)
        controlText = "正在实测校准";
    else if (s.capturing)
        controlText = "采集中，侧键跟随暂停";
    else if (s.device.testArmed)
        controlText = "F8桌面测试中";
    else if (!s.motion.aimEnabled)
        controlText = "Aimbot已关闭";
    else if (s.suspended)
        controlText = "已暂停，等待同一人物恢复 · " + blockText(report, s);
    else if (s.releaseRequired)
        controlText = "请松开侧键再按 · " + motionEventText(s.stoppedBy);
    else if (s.controlActive)
        controlText = "正在跟随，松开侧键停止";
    else if (s.aimArmed && !s.device.triggerAvailable)
        controlText = "等待新鲜侧键回传";
    else
        controlText =
            report.block == control::LiveBlock::Ready ? "已就绪，按住侧键跟随" : "未输出 · " + blockText(report, s);
    Json profiles = calibrationRecordsJson(s.profiles);
    std::string profileText;
    for (const auto &r : s.profiles)
    {
        if (!profileText.empty())
            profileText += " / ";
        profileText += modeName(r.mode) + " " + number(r.value.fov, 1) + "°";
    }
    const auto predicted =
        report.target ? control::predictPosition(*report.target, now) : control::PositionPrediction{};
    const auto weaponAim = s.living ? control::resolveWeaponAim(*s.living, s.motion.ballisticCatalog.get(),
                                                                s.motion.aimPart, s.motion.weaponAim)
                                    : control::WeaponAimSelection{s.motion.aimPart};
    Json state = {
        {"automaticConnectionIntervalMs", AutomaticConnectionIntervalMs},
        {"readerMetrics", describeReaderMetrics(s.readerMetrics, gpu.initialized && gpu.gpuCompletionAvailable)},
        {"connections", {{"reader", describeReaderConnection(s, now)}}},
        {"sessionEligible", context.sessionEligible},
        {"livingConnected", s.livingConnected},
        {"localConnected", s.viewConnected},
        {"readerRunning", s.readerRunning},
        {"readerConnected", s.readerRunning && s.livingConnected && s.viewConnected},
        {"aimArmed", s.aimArmed},
        {"aimAutomatic", s.aimArmed},
        {"effectiveAimPart", static_cast<unsigned>(weaponAim.part)},
        {"weaponAimCategory", weaponAim.category ? Json(control::weaponCategoryKey(*weaponAim.category)) : Json{}},
        {"weaponAimOverride", weaponAim.overridden},
        {"calibrating", s.calibrating},
        {"capturing", s.capturing},
        {"canAim", context.sessionEligible && s.motion.aimEnabled && report.block == control::LiveBlock::Ready},
        {"canCalibrate",
         context.sessionEligible && s.device.boxReady && s.readerRunning && !s.calibrating && !s.capturing},
        {"source", s.source},
        {"characters", s.living ? Json(s.living->characters.size()) : Json{}},
        {"target", report.target ? Json(std::to_string(report.target->identity.id)) : Json{}},
        {"gap", s.living && s.view ? Json(report.pair.sourceSkewMs) : Json{}},
        {"fov", s.view ? nullable(s.view->fov) : Json{}},
        {"angle", report.target ? Json(report.target->error.cone) : Json{}},
        {"targetDistance",
         report.target ? Json(report.target->error.distanceUnits / control::WorldUnitsPerMeter) : Json{}},
        {"control", controlText},
        {"calibration", calibrationText(s)},
        {"profiles", profileText.empty() ? "暂无实测记录" : profileText},
        {"calibrationRecords", profiles},
        {"prediction", predicted.applied() ? "估算数据年龄 " + number(predicted.ageMs, 1) + " ms（±" +
                                                 number(predicted.uncertaintyMs, 1) + " ms）"
                                           : "等待当前人物速度与时钟"},
        {"speedHint", "最高 " + number(control::speedPolicy(s.speed, s.device.motionLimits).maximumCountsPerSecond, 0) +
                          " counts/s · 短S曲线 · 实时FOV"},
        {"ballistics", ballisticBlockText(report.ballistics.block)},
        {"ballisticsError", context.ballisticsError},
        {"ballisticProfile", report.ballistics.profileName},
        {"bulletFlightMs", report.ballistics.block == control::BallisticBlock::Ready
                               ? Json(report.ballistics.solution.flightSeconds * 1000)
                               : Json{}},
        {"bulletDropMeters", report.ballistics.block == control::BallisticBlock::Ready
                                 ? Json(report.ballistics.solution.dropMeters)
                                 : Json{}},
        {"localBodyKnown", s.view && s.view->self.has_value()},
        {"hasCurrentItem", s.view && s.view->currentItem && s.view->currentItem->hasCurrentItem
                               ? Json(*s.view->currentItem->hasCurrentItem)
                               : Json{}},
        {"currentWeaponClass", s.view && s.view->activeWeapon ? Json(s.view->activeWeapon->actorClass) : Json{}},
        {"spectators", s.view && s.view->spectatedCount ? Json(*s.view->spectatedCount) : Json{}},
        {"espVisible",
         "可见 " + std::to_string(gpu.esp.counts.visible) + " / 人物 " + std::to_string(gpu.esp.counts.input)},
        {"espDrawFramesPerSecond", gpu.drawFramesPerSecond},
        {"espSequences",
         "Local #" + std::to_string(gpu.esp.viewCapture) + " · Living #" + std::to_string(gpu.esp.livingCapture)},
        {"espFov", gpu.esp.fov ? "FOV " + number(gpu.esp.fov, 1) : "FOV —"},
        {"espUnits", gpu.esp.units == render::EspUnits::Unknown ? "单位未核验" : "单位来源已标注"}};
    const bool nativeActive = s.controlActive && s.motionFeedback && s.motionFeedback->nativeReference;
    state["aimReference"] = s.controlActive ? (nativeActive ? "native" : "camera") : "pending";
    state["nativeZeroingMeters"] =
        nativeActive && s.view && s.view->nativeAimBasis && s.view->nativeAimBasis->zeroingDistanceMeters
            ? Json(*s.view->nativeAimBasis->zeroingDistanceMeters)
            : Json{};
    return state;
}
} // namespace mousebox::application
