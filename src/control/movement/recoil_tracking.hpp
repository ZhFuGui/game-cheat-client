#pragma once
#include "smooth.hpp"
namespace mousebox::control::movement
{
class RecoilTracking
{
    Smooth tracking_;
    std::optional<Input> previous_;
    Vector debt_;
    double tokensX_ = 0, tokensY_ = 0;

  public:
    Result step(const Input &, HumanMotionRuntime &);
};
} // namespace mousebox::control::movement
