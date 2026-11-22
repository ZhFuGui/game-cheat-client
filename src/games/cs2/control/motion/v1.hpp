#pragma once
#include "kinematic.hpp"
namespace mousebox::games::cs2::motion
{
// PC2.2.4 motion baseline. New experiments must not change this law.
class V1
{
    Kinematic engine_;

  public:
    Result step(const Input &, control::HumanMotionRuntime &, std::optional<Counts> caps = {}, MotionFactors = {});
};
} // namespace mousebox::games::cs2::motion
