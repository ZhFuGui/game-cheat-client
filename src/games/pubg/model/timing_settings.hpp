#pragma once
#include <cmath>

namespace mousebox::control
{
inline constexpr double DefaultLivingLocalSkewMs = 100;
inline constexpr double MaximumLivingLocalSkewMs = 5000;

inline bool validLivingLocalSkew(double value) noexcept
{
    return std::isfinite(value) && value >= 0 && value <= MaximumLivingLocalSkewMs && std::floor(value) == value;
}
} // namespace mousebox::control
