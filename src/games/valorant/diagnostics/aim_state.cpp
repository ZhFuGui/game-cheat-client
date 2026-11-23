#include "aim_state.hpp"
#include <map>

namespace mousebox::games::valorant
{
void describeAim(application::Json &state, const aim::Status &s)
{
    using application::Json;
    static const std::map<std::string, std::string> Messages{
        {"disabled", "Aimbot已关闭"},
        {"waiting-source", "等待本地人物、骨骼和透视相机"},
        {"invalid-settings", "瞄准参数无效"},
        {"waiting-foreground", "等待本机游戏窗口处于前台"},
        {"local-unavailable", "等待已确认的本地人物"},
        {"manual-view-unconfirmed", "请确认当前相机为本地可控视角"},
        {"mapping-unavailable", "请填写非零的横纵角度/鼠标计数"},
        {"source-mismatch", "等待同一Reader与会话的数据"},
        {"source-timing-unavailable", "等待新鲜数据和可信时钟对齐"},
        {"source-skew", "三路采样时间差过大，等待接近的采样"},
        {"camera-unavailable", "当前相机不可用于跟随"},
        {"no-target", "等待角差范围内所选部位的骨点"},
        {"held-target-dead", "目标已确认死亡，松开侧键后重新选择"},
        {"held-target-unavailable", "本次锁定的目标暂不可用"},
        {"held-target-missing", "等待本次锁定的目标恢复"},
        {"release-then-hold", "松开后按住所设侧键开始跟随"},
        {"waiting-side-key", "计算就绪，按住侧键跟随"},
        {"button-stale", "等待新鲜侧键回传"},
        {"device-unavailable", "等待设备运动能力"},
        {"clock-regressed", "采样顺序异常，等待新采样"},
        {"scheduler-stall", "调度暂停，正在恢复"},
        {"feedback-timeout", "等待相机角度响应，恢复后继续同一目标"},
        {"feedback-delayed", "等待相机反馈消化已提交位移"},
        {"output-rejected", "本次位移未提交，正在重新核验"},
        {"tracking", "正在按手动换算跟随所选骨点"}};
    const auto found = Messages.find(s.block);
    state["control"] = found == Messages.end() ? s.block : found->second;
    state["canAim"] = s.settings.enabled && s.solution.ready();
    state["aimAutomatic"] = true;
    const auto target = s.held ? s.held : s.solution.target;
    state["target"] = target ? Json(std::to_string(*target)) : Json{};
    state["valorantAim"] = {{"enabled", s.settings.enabled},
                            {"active", s.active},
                            {"block", s.block},
                            {"basis", "manual-camera-feedback"},
                            {"manualViewConfirmed", s.manualViewConfirmed},
                            {"canConfirmView", s.canConfirm},
                            {"heldTarget", s.held ? Json(std::to_string(*s.held)) : Json{}},
                            {"pending", s.pending},
                            {"sampleAgeMs", s.solution.sampleMs > 0 ? Json(s.solution.ageMs) : Json{}},
                            {"distanceUnits", s.solution.target ? Json(s.solution.distance) : Json{}},
                            {"lifeStateKnown", s.solution.lifeKnown},
                            {"yawPerCount", s.settings.yawPerCount ? Json(s.settings.yawPerCount) : Json{}},
                            {"pitchPerCount", s.settings.pitchPerCount ? Json(s.settings.pitchPerCount) : Json{}}};
}
} // namespace mousebox::games::valorant
