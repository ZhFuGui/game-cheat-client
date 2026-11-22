#pragma once
#include "games/cs2/model/snapshot.hpp"
#include "games/cs2/control/policy.hpp"
namespace mousebox::games::cs2::pipeline
{
std::optional<Vec3> aimPoint(const Body &, unsigned part);
bool onScreen(const Snapshot &, Vec3);
struct TargetGeometry
{
    Vec3 point;
    Angles desired;
    Vec3 origin;
};
TargetGeometry predictGeometry(Vec3 point, Vec3 eye, Vec3 targetVelocity, Vec3 selfVelocity, double horizonMs,
                               bool prediction, GeometryFactors factors = {});
} // namespace mousebox::games::cs2::pipeline
