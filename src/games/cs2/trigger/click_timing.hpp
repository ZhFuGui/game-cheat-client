#pragma once
#include "domain/random_range.hpp"

namespace mousebox::games::cs2::trigger
{
inline constexpr random::IntegerRange IntervalBounds{50, 1000}, HoldBounds{5, 40};
inline constexpr random::IntegerRange DefaultInterval{90, 110}, DefaultHold{15, 25};
} // namespace mousebox::games::cs2::trigger
