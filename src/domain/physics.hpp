#pragma once
#include <cmath>

namespace mousebox::control
{
inline constexpr double DefaultGravityMetersPerSecondSquared = 9.8;
inline constexpr double MaximumGravityMetersPerSecondSquared = 100;
inline bool validGravity(double value) noexcept
{
    return std::isfinite(value) && value >= 0 && value <= MaximumGravityMetersPerSecondSquared;
}
} // namespace mousebox::control
