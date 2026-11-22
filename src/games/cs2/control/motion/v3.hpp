#pragma once
#include "types.hpp"
#include "control/movement/direct.hpp"
namespace mousebox::games::cs2::motion
{
// Absolute residual-angle experiment: no tracking gain, acceleration or noise.
class V3
{
    control::movement::Direct engine_;

  public:
    Result step(const Input &, control::HumanMotionRuntime &);
};
} // namespace mousebox::games::cs2::motion
