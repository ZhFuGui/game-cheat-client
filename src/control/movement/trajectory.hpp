#pragma once
#include <array>
#include <string_view>

namespace mousebox::control::movement
{
enum class TrajectoryKind
{
    Linear,
    Bezier
};
struct TrajectorySettings
{
    TrajectoryKind kind = TrajectoryKind::Linear;
    // Chord-relative fractions: X follows the chord; Y is its perpendicular.
    double firstX = .25, firstY = .20, secondX = .75, secondY = .20;
    bool operator==(const TrajectorySettings &) const = default;
    bool valid() const noexcept;
};
struct PathPoint
{
    double x = 0, y = 0;
};
PathPoint cubicBezier(PathPoint first, PathPoint second, PathPoint end, double t) noexcept;

// Geometry only. The caller supplies a distance from its speed controller and
// commits the resulting counts through its existing output ledger. Copying this
// bounded state is enough to roll back a rejected proposal.
class BezierPath
{
    static constexpr unsigned Segments = 48;
    TrajectorySettings settings_;
    PathPoint position_, initialEnd_, first_, second_;
    std::array<PathPoint, Segments + 2> points_{};
    std::array<double, Segments + 2> parameters_{}, lengths_{};
    double t_ = 0, length_ = 0;
    bool initialized_ = false;

  public:
    void reset() noexcept
    {
        *this = {};
    }
    double prepare(PathPoint remaining, const TrajectorySettings &);
    PathPoint advance(double distance, unsigned maximumStep);
    PathPoint chordDirection() const noexcept;
    double progress() const noexcept
    {
        return t_;
    }
};
} // namespace mousebox::control::movement
