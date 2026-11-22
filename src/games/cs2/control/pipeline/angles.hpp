#pragma once
#include "games/cs2/model/geometry.hpp"
#include <cmath>
namespace mousebox::games::cs2::pipeline
{
inline bool finite(Vec3 v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
inline Angles angularError(Angles desired, Angles direction)
{
    return {std::remainder(desired.x - direction.x, 360), std::remainder(desired.y - direction.y, 360), 0};
}
} // namespace mousebox::games::cs2::pipeline
