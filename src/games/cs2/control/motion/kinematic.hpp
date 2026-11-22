#pragma once
#include "types.hpp"
#include "control/movement/smooth.hpp"
namespace mousebox::games::cs2::motion
{
// One integrator with independently selectable constraints; baseline enables all.
class Kinematic
{
    control::movement::Smooth engine_;

  public:
    Result step(const Input &, control::HumanMotionRuntime &, std::optional<Counts>, MotionFactors);
};
} // namespace mousebox::games::cs2::motion
