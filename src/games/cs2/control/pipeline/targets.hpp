#pragma once
#include "frame_context.hpp"
#include "geometry.hpp"
namespace mousebox::games::cs2::pipeline
{
void selectTarget(AimSolution &, const Snapshot &, const Body &, const AimSettings &, std::optional<TargetKey> held,
                  std::optional<TargetPoint> = {}, const AimPointCheck & = {}, const AimPointResolver & = {});
} // namespace mousebox::games::cs2::pipeline
