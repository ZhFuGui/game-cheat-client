#pragma once
#include "aim_types.hpp"
#include "pipeline/frame_context.hpp"
namespace mousebox::games::cs2
{
AimSolution solveAim(const ControlInput &, const AimSettings &, std::uint64_t nowMs, std::optional<TargetKey> held = {},
                     const AimPointCheck &check = {}, const AimPointResolver &resolvePoint = {});
AimSolution solveAimAt(const ControlInput &, const AimSettings &, std::uint64_t nowMs, const TargetPoint &);
AimSolution solveManualAimReference(const ControlInput &, const AimSettings &, std::uint64_t nowMs);
} // namespace mousebox::games::cs2
