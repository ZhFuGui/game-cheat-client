#pragma once
#include <array>
#include <string>
#include <string_view>
namespace mousebox::games::cs2
{
inline std::string controlMessage(std::string_view code)
{
    using Pair = std::pair<std::string_view, std::string_view>;
    constexpr std::array messages{
        Pair{"disabled", "Aimbot已关闭"},
        Pair{"waiting-source", "等待 CS2 当前快照"},
        Pair{"unsupported-build", "当前 CS2 Build 未核验"},
        Pair{"source-unverified", "本帧人物目录尚未通过来源核验"},
        Pair{"local-not-playing", "等待普通本地第一人称存活角色"},
        Pair{"eye-missing", "等待当前眼部位置和输入角度"},
        Pair{"mouse-mapping-missing", "等待有效鼠标原生映射"},
        Pair{"clock-unavailable", "采样过期或时钟映射不可信"},
        Pair{"eye-context-missing", "眼角与后坐原生上下文缺失或不一致"},
        Pair{"recoil-unavailable", "后坐状态不能求值"},
        Pair{"team-unknown", "己方队伍未知"},
        Pair{"local-velocity-missing", "等待己方世界速度"},
        Pair{"target-velocity-missing", "等待目标世界速度"},
        Pair{"no-target", "没有符合条件的目标"},
        Pair{"targets-disabled", "未选择包含对象"},
        Pair{"aim-hitbox-unavailable", "等待所选部位的有效 Hitbox 姿态"},
        Pair{"cover-pending", "等待瞄准部位的掩体检测资源"},
        Pair{"cover-occluded", "所选瞄准部位被掩体遮挡，暂停跟随"},
        Pair{"cover-searching", "正在寻找露出的 Hitbox 瞄点"},
        Pair{"cover-unknown", "无法确定瞄准部位是否露出，暂停跟随"},
        Pair{"cover-map-missing", "等待当前地图信息，暂停跟随"},
        Pair{"cover-unavailable", "掩体检测资源不可用，暂停跟随"},
        Pair{"held-target-missing", "锁定目标暂不可用，等待同一目标恢复"},
        Pair{"held-target-dead", "锁定目标已死亡，松开侧键后重新选人"},
        Pair{"ready", "计算就绪"},
        Pair{"waiting-side-key", "计算就绪，按住侧键跟随"},
        Pair{"tracking", "正在跟随锁定目标"},
        Pair{"recoil-ready", "无可用目标，按住侧键可补偿后坐"},
        Pair{"recoil-only", "正在补偿后坐，瞄准方向由鼠标控制"},
        Pair{"rage-selected", "Rage已接管输出，Aimbot暂停"},
        Pair{"release-then-hold", "请先松开侧键再按"},
        Pair{"calibration-required", "实测模式需要有效校准记录，请校准或切回原生公式"},
        Pair{"device-unavailable", "等待已握手的设备能力"},
        Pair{"context-changed", "运行上下文已变化，请松开侧键再按"},
        Pair{"clock-regressed", "采集顺序或时间回退，等待可信新帧后恢复同一目标"},
        Pair{"scheduler-stall", "调度短暂中断，恢复后继续同一目标"},
        Pair{"feedback-timeout", "未观察到实际输入角响应，保留目标并等待响应恢复"},
        Pair{"feedback-delayed", "等待采样反映已发位移，保留同一目标"},
        Pair{"output-rejected", "当前输出被撤销或拒绝"},
        Pair{"button-stale", "侧键回传失效，已停止"},
        Pair{"calibrating", "正在进行设备计数校准"},
        Pair{"session-ineligible", "运行会话未获资格，输出已停止"},
        Pair{"eye-input-transform-unverified", "父变换或委托角度已读取，局部鼠标到世界方向的控制尚未核验"}};
    for (const auto &[key, text] : messages)
        if (key == code)
            return std::string(text);
    return std::string(code);
}
} // namespace mousebox::games::cs2
