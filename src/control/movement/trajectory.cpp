#include "trajectory.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace mousebox::control::movement
{
bool TrajectorySettings::valid() const noexcept
{
    const auto in = [](double v, double lo, double hi) { return std::isfinite(v) && v >= lo && v <= hi; };
    return (kind == TrajectoryKind::Linear || kind == TrajectoryKind::Bezier) && in(firstX, 0, 1) &&
           in(secondX, 0, 1) && in(firstY, -1, 1) && in(secondY, -1, 1);
}
PathPoint cubicBezier(PathPoint a, PathPoint b, PathPoint end, double t) noexcept
{
    const double u = 1 - t, f = 3 * u * u * t, s = 3 * u * t * t, e = t * t * t;
    return {f * a.x + s * b.x + e * end.x, f * a.y + s * b.y + e * end.y};
}
double BezierPath::prepare(PathPoint remaining, const TrajectorySettings &settings)
{
    if (!settings.valid() || !std::isfinite(remaining.x) || !std::isfinite(remaining.y))
        throw std::invalid_argument("无效鼠标轨迹参数");
    if (std::hypot(remaining.x, remaining.y) <= .5)
    {
        reset();
        return 0;
    }
    if (!initialized_ || settings_ != settings)
    {
        reset();
        settings_ = settings;
        initialEnd_ = remaining;
        first_ = {remaining.x * settings.firstX - remaining.y * settings.firstY,
                  remaining.y * settings.firstX + remaining.x * settings.firstY};
        second_ = {remaining.x * settings.secondX - remaining.y * settings.secondY,
                   remaining.y * settings.secondX + remaining.x * settings.secondY};
        initialized_ = true;
    }
    const PathPoint end{position_.x + remaining.x, position_.y + remaining.y};
    // Moving-target corrections shift the terminal handle and endpoint together;
    // the initial tangent stays stable. No old endpoint is ever sent blindly.
    const PathPoint second{second_.x + end.x - initialEnd_.x, second_.y + end.y - initialEnd_.y};
    points_[0] = position_;
    parameters_[0] = t_;
    lengths_[0] = 0;
    for (unsigned i = 0; i <= Segments; ++i)
    {
        const double t = t_ + (1 - t_) * i / Segments;
        const auto point = cubicBezier(first_, second, end, t);
        points_[i + 1] = point;
        parameters_[i + 1] = t;
        lengths_[i + 1] = lengths_[i] + std::hypot(point.x - points_[i].x, point.y - points_[i].y);
    }
    length_ = lengths_.back();
    if (!std::isfinite(length_))
        throw std::invalid_argument("轨迹长度超出数值范围");
    return length_;
}
PathPoint BezierPath::advance(double distance, unsigned maximumStep)
{
    if (!std::isfinite(distance) || distance < 0 || !maximumStep)
        throw std::invalid_argument("无效轨迹推进距离");
    if (!initialized_ || !length_ || !distance)
        return {};
    distance = std::min(distance, length_);
    unsigned index = 1;
    while (index < lengths_.size() - 1 && lengths_[index] < distance)
        ++index;
    const double segment = lengths_[index] - lengths_[index - 1];
    const double fraction = segment > 0 ? (distance - lengths_[index - 1]) / segment : 1;
    const auto interpolate = [&](double a, double b) { return a + (b - a) * fraction; };
    const PathPoint desired{interpolate(points_[index - 1].x, points_[index].x),
                            interpolate(points_[index - 1].y, points_[index].y)};
    const double cap = maximumStep;
    const PathPoint counts{std::round(std::clamp(desired.x - position_.x, -cap, cap)),
                           std::round(std::clamp(desired.y - position_.y, -cap, cap))};
    position_.x += counts.x;
    position_.y += counts.y;
    t_ = interpolate(parameters_[index - 1], parameters_[index]);
    return counts;
}
PathPoint BezierPath::chordDirection() const noexcept
{
    const double length = std::hypot(initialEnd_.x, initialEnd_.y);
    return length > 0 ? PathPoint{initialEnd_.x / length, initialEnd_.y / length} : PathPoint{1, 0};
}
} // namespace mousebox::control::movement
