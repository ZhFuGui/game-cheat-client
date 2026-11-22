#pragma once
#include "types.hpp"
namespace mousebox::control::movement
{
class Direct
{
    double tokensX_ = 0, tokensY_ = 0;

  public:
    Result step(const Input &, HumanMotionRuntime &);
};
} // namespace mousebox::control::movement
