#pragma once
#include <cmath>

namespace mousebox::domain
{
struct Vec3
{
    double x = 0, y = 0, z = 0;
};
inline bool finite(Vec3 value) noexcept
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}
} // namespace mousebox::domain

namespace mousebox::control
{
using domain::finite;
using domain::Vec3;
} // namespace mousebox::control
