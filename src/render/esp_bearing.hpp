#pragma once
#include "primitives.hpp"
#include "domain/math.hpp"
namespace mousebox::render
{
BearingRing bearingLayout(float width, float height);
// Horizontal camera-relative bearing only: all people occupy the same radius.
// Pitch, roll, FOV, elevation and world-unit scale cannot encode distance here.
void appendBearing(BearingRing &, domain::Vec3 camera, domain::Vec3 person, double yawDegrees, unsigned group,
                   std::uint64_t id);
} // namespace mousebox::render
