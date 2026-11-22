#pragma once
#include "control/human_runtime.hpp"
#include "control/motion_policy.hpp"
#include "factors.hpp"
#include <optional>
namespace mousebox::control::movement
{
// Movement coordinates are always horizontal X and vertical Y. Games adapt
// their angle conventions before this boundary; strategies never read a game.
struct Vector
{
    double x = 0, y = 0;
};
struct Counts
{
    int x = 0, y = 0;
};
struct Input
{
    Vector errorDegrees, degreesPerCount, recoilDegrees;
    double dt = 0;
    MotionPolicy policy;
    std::uint64_t now = 0, capture = 0;
    std::int64_t sourceStartNs = 0;
    double simulationTicks = 0, secondsPerTick = 0, uncertaintyMs = 0;
    bool recoilEnabled = false, recoilDeltaQualified = false;
    // Angular X normally wraps at 360 degrees. An arc-length adapter supplies
    // an unwrapped scalar coordinate and explicitly sets this period to zero.
    double recoilHorizontalPeriod = 360;
};
struct Result
{
    Counts counts, fast;
    const char *status = "v1";
};
} // namespace mousebox::control::movement
