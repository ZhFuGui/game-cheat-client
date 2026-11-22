#pragma once
#include "runtime_models.hpp"
#include <string>
namespace mousebox::application
{
inline std::string aimPartName(control::AimPart part)
{
    constexpr const char *names[] = {"头部", "颈部", "胸部", "腹部", "骨盆"};
    const auto n = static_cast<unsigned>(part);
    return n < 5 ? names[n] : "未知部位";
}
inline std::string modeName(control::ViewMode mode)
{
    constexpr const char *names[] = {"第三人称", "第一人称", "瞄准", "开镜"};
    const auto n = static_cast<unsigned>(mode);
    return n < 4 ? names[n] : "未知模式";
}
inline std::string calibrationText(const Snapshot &s)
{
    if (s.calibrating)
        return s.calibrationStatus;
    if (s.calibration.formulaEstimated)
        return "实时 FOV 公式估算：yaw " + std::to_string(s.calibration.yawDegreesPerCount) + " / pitch " +
               std::to_string(s.calibration.pitchDegreesPerCount) + " deg/count";
    if (s.calibrationMatch.record)
        return "已匹配实测校准：" + modeName(s.calibrationMatch.record->mode);
    return s.inputMapping == control::InputMapping::DynamicFov ? "实时 FOV 换算等待有效视点" : "等待匹配的实测校准";
}
inline std::string ballisticBlockText(control::BallisticBlock block)
{
    using B = control::BallisticBlock;
    switch (block)
    {
    case B::Disabled:
        return "等待当前目标与弹道输入";
    case B::Ready:
        return "提前量、下坠与归零解算可用";
    case B::CatalogMissing:
        return "弹道资料包未就绪";
    case B::WeaponMissing:
        return "等待本地当前手持武器身份";
    case B::CameraMissing:
        return "等待有效相机位置与视角";
    case B::TargetVelocityMissing:
        return "等待目标完整身体世界速度";
    case B::SelfVelocityMissing:
        return "等待自己完整身体世界速度";
    case B::TimingUnavailable:
        return "目标或相机时钟与源时间未对齐";
    case B::UnknownWeapon:
        return "当前武器未匹配公开资料";
    case B::UnsupportedWeapon:
        return "武器资料歧义或无有效弹速";
    case B::NoSolution:
        return "当前弹道模型无解";
    case B::ZeroingUnreachable:
        return "归零距离超出模型范围";
    case B::InvalidConfiguration:
        return "弹道设置无效";
    }
    return "弹道数据不可用";
}
inline std::string blockText(const control::LiveDiagnostic &d, const Snapshot &s)
{
    using B = control::LiveBlock;
    switch (d.block)
    {
    case B::Ready:
        return "目标与输入换算已就绪";
    case B::NotRealtime:
        return "请连接 Reader，离线预览不会输出";
    case B::DeviceUnavailable:
        return "请连接鼠标盒子";
    case B::LivingMissing:
        return "等待 Living 当前人物数据";
    case B::ViewMissing:
        return "等待 Local 当前视点数据";
    case B::PairRejected:
        return "人物和视点上下文不可组合：" + std::string(control::reasonName(d.pair.reason));
    case B::LivingStale:
        return "Living 源采样超过800ms未前进";
    case B::ViewStale:
        return "Local 源采样超过800ms未前进";
    case B::CalibrationMissing:
        return calibrationText(s);
    case B::FovMismatch:
        return "当前FOV与实测校准不匹配";
    case B::SelfTeamMissing:
        return "Reader尚未提供当前队伍";
    case B::NoTarget:
        return "范围内无合格目标";
    case B::BallisticsUnavailable:
        return ballisticBlockText(d.ballistics.block);
    case B::RotationTimingUnavailable:
        return "相机旋转的独立采样时间不可用、过期或时钟误差过大";
    case B::NativeReferenceUnavailable:
        return "已锁定的原生方向或控制器输入轴暂不可用，保留目标并暂停";
    }
    return "等待数据";
}
inline std::string motionEventText(control::MotionEvent event)
{
    using E = control::MotionEvent;
    switch (event)
    {
    case E::Released:
        return "触发键已松开";
    case E::Suspended:
        return "锁定人物暂不可用，保持侧键等待同一人物恢复";
    case E::Resumed:
        return "同一锁定人物恢复";
    case E::EligibilityLost:
        return "目标或输出条件失效";
    case E::ContextChanged:
        return "Reader上下文或校准配置切换";
    case E::TargetChanged:
        return "锁定目标改变";
    case E::ClockRegressed:
        return "本机计时倒退";
    case E::TimeLimit:
        return "运动计划时长上限";
    case E::SchedulerStall:
        return "控制线程超过30ms未调度";
    case E::CameraUnconfirmed:
        return "800ms未观察到相机进展，请松键检查链路";
    case E::CountLimit:
        return "运动计划计数上限";
    case E::RejectedPress:
        return "按下时输出条件未就绪";
    default:
        return "已停止";
    }
}
} // namespace mousebox::application
