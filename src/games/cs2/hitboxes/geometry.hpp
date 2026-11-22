#pragma once
#include "pose.hpp"
#include <array>

namespace mousebox::games::cs2::hitboxes
{
using Triangle = std::array<Vec3, 3>;
// A deterministic tessellation of the resource shape; no anatomical estimates.
std::vector<Triangle> localSurface(const Definition &);
std::vector<Triangle> worldSurface(const PosedHitbox &);
} // namespace mousebox::games::cs2::hitboxes
