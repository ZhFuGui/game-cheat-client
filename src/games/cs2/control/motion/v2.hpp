#pragma once
#include "v1.hpp"
#include "control/movement/recoil_tracking.hpp"
namespace mousebox::games::cs2::motion
{
class V2
{
    control::movement::RecoilTracking engine_;

  public:
    Result step(const Input &, control::HumanMotionRuntime &);
};
} // namespace mousebox::games::cs2::motion
