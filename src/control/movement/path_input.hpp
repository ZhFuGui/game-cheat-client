#pragma once
#include "types.hpp"
#include "trajectory.hpp"
namespace mousebox::control::movement
{
// Adapt angular residuals to a one-dimensional remaining arc. Existing temporal
// controllers decide how far to advance; they cannot alter the curve's shape.
Input pathInput(const Input &, BezierPath &);
Result pathResult(const Result &, const Input &, BezierPath &);
} // namespace mousebox::control::movement
