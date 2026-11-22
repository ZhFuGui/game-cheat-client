#pragma once
namespace mousebox::games::cs2
{
struct Vec3
{
    double x = 0, y = 0, z = 0;
};
using Angles = Vec3; // x=pitch, y=yaw, z=roll; original degrees.
} // namespace mousebox::games::cs2
