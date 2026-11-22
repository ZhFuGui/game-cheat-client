#include "path_input.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace mousebox::control::movement
{
Input pathInput(const Input &in, BezierPath &path)
{
    if (!std::isfinite(in.degreesPerCount.x) || !std::isfinite(in.degreesPerCount.y) || !in.degreesPerCount.x ||
        !in.degreesPerCount.y)
        throw std::invalid_argument("曲线缺少有效设备计数换算");
    auto scalar = in;
    const PathPoint remaining{in.errorDegrees.x / in.degreesPerCount.x, in.errorDegrees.y / in.degreesPerCount.y};
    scalar.errorDegrees = {path.prepare(remaining, in.policy.trajectory), 0};
    // A fixed chord basis avoids synthesizing recoil deltas merely because the
    // path's tangent turns. Native recoil evaluation itself remains unchanged.
    const auto direction = path.chordDirection();
    scalar.recoilDegrees = {in.recoilDegrees.x / in.degreesPerCount.x * direction.x +
                                in.recoilDegrees.y / in.degreesPerCount.y * direction.y,
                            0};
    scalar.degreesPerCount = {1, 1};
    scalar.recoilHorizontalPeriod = 0;
    return scalar;
}
Result pathResult(const Result &scalar, const Input &in, BezierPath &path)
{
    auto result = scalar;
    const auto counts = path.advance(std::max(0, scalar.counts.x), in.policy.maximumStepCounts);
    result.counts = {static_cast<int>(counts.x), static_cast<int>(counts.y)};
    const double fastRatio = scalar.counts.x ? static_cast<double>(scalar.fast.x) / scalar.counts.x : 0;
    result.fast = {static_cast<int>(std::trunc(result.counts.x * fastRatio)),
                   static_cast<int>(std::trunc(result.counts.y * fastRatio))};
    return result;
}
} // namespace mousebox::control::movement
