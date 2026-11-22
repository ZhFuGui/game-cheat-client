#pragma once
#include "types.hpp"
namespace mousebox::control::movement
{
// The preserved baseline integrator, independent of targeting and recoil evaluation.
class Smooth
{
    double vx_ = 0, vy_ = 0, fx_ = 0, fy_ = 0;

  public:
    Result step(const Input &, HumanMotionRuntime &, std::optional<Counts> caps = {}, Factors = {});
};
} // namespace mousebox::control::movement
