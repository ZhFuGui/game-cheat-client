#pragma once
namespace mousebox::control::movement
{
struct Factors
{
    bool gainLimit = true, acceleration = true, braking = true, variation = true, learnedLimits = true;
    bool operator==(const Factors &) const = default;
};
} // namespace mousebox::control::movement
