#pragma once
#include "movement/trajectory.hpp"

namespace mousebox::control
{
struct MotionPolicy
{
    double gain = 8, maximumCountsPerSecond = 400, acceleration = 2000, deadZoneDegrees = 0.05;
    unsigned budget = 32, durationMs = 320, intervalMs = 10, maximumStepCounts = 4;
    double maximumJerk = 0;
    movement::TrajectorySettings trajectory;
};
} // namespace mousebox::control
