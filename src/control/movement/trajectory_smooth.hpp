#pragma once
#include "types.hpp"
namespace mousebox::control::movement
{
struct TrajectoryInput
{
    Input movement;
    Vector targetVelocity;
    double deadZoneDegrees = 0, trackingGain = 20;
    bool geometryAvailable = false, tracking = false;
};
// Existing jerk-limited acquisition and bounded tracking servo. This module
// owns only trajectory state; source feedback and target geometry stay outside.
class TrajectorySmooth
{
    double vx_ = 0, vy_ = 0, fx_ = 0, fy_ = 0, ax_ = 0, ay_ = 0;
    BezierPath path_;
    struct State
    {
        double vx, vy, fx, fy, ax, ay;
        BezierPath path;
    };
    std::optional<State> rollback_;
    std::optional<Counts> stepLinear(const TrajectoryInput &, HumanMotionRuntime &);

  public:
    void reset() noexcept
    {
        *this = {};
    }
    std::optional<Counts> step(const TrajectoryInput &, HumanMotionRuntime &);
    void reject() noexcept;
};
} // namespace mousebox::control::movement
