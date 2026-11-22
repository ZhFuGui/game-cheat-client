#pragma once
namespace mousebox::control
{
enum class MotionEvent
{
    None,
    Begin,
    Released,
    RejectedPress,
    EligibilityLost,
    ContextChanged,
    TargetChanged,
    ClockRegressed,
    TimeLimit,
    SchedulerStall,
    CountLimit,
    CameraUnconfirmed,
    Suspended,
    Resumed
};
struct MotionStep
{
    bool begin = false, stop = false;
    int dx = 0, dy = 0;
    unsigned spent = 0;
    MotionEvent event = MotionEvent::None;
};
} // namespace mousebox::control
