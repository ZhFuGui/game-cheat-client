#include "esp_scale.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace mousebox::render
{
namespace
{
double length(domain::Vec3 a)
{
    return std::hypot(a.x, a.y, a.z);
}
} // namespace
void SkeletonReference::point(domain::Vec3 p)
{
    if (!domain::finite(p))
        return;
    const domain::Vec3 d{p.x - mean_.x, p.y - mean_.y, p.z - mean_.z};
    const auto n = static_cast<double>(++count_);
    mean_ = {mean_.x + d.x / n, mean_.y + d.y / n, mean_.z + d.z / n};
    variance_ += d.x * (p.x - mean_.x) + d.y * (p.y - mean_.y) + d.z * (p.z - mean_.z);
}
void SkeletonReference::edge(domain::Vec3 a, domain::Vec3 b, bool headNeck)
{
    const auto size = length({a.x - b.x, a.y - b.y, a.z - b.z});
    if (!std::isfinite(size) || size <= 0)
        return;
    longestEdge_ = std::max(longestEdge_, size);
    if (headNeck)
        headSpan_ = std::max(headSpan_, size);
}
double SkeletonReference::span() const noexcept
{
    if (headSpan_ > 0)
        return headSpan_;
    if (longestEdge_ > 0)
        return longestEdge_ * .25;
    if (count_ > 1 && variance_ > 0)
        return std::sqrt(variance_ / static_cast<double>(count_)) * .3;
    // A lone point has no model dimension. One source-world unit is a styling
    // fallback, not an assertion about physical meters; it still projects.
    return 1;
}
EspScale espScale(double pixelsPerWorld, double referenceSpan) noexcept
{
    const double span = pixelsPerWorld * referenceSpan;
    if (!std::isfinite(span) || span <= 0 || span > std::numeric_limits<float>::max())
        return {};
    const auto scale = static_cast<float>(span / 16);
    // Real geometry has no fixed-pixel minimum. Attached information follows a
    // gentler continuous curve: 0.85..1.10, exactly 1 at a 16-DIP reference span.
    return {2.5f * scale, 1.5f * scale, static_cast<float>(.85 + .25 * span / (span + 32. / 3))};
}
MatrixProjectionScale::MatrixProjectionScale(const std::array<double, 16> &m, float w, float h) noexcept
{
    if (!std::isfinite(w) || !std::isfinite(h) || w <= 0 || h <= 0 ||
        !std::ranges::all_of(m, [](double v) { return std::isfinite(v); }))
        return;
    depth_ = {m[12], m[13], m[14], m[15]};
    const domain::Vec3 forward{m[12], m[13], m[14]};
    const double n = length(forward);
    const auto tangentLength = [&](domain::Vec3 row) {
        if (n > 0)
        {
            const domain::Vec3 normal{forward.x / n, forward.y / n, forward.z / n};
            const double d = row.x * normal.x + row.y * normal.y + row.z * normal.z;
            row = {row.x - d * normal.x, row.y - d * normal.y, row.z - d * normal.z};
        }
        return length(row);
    };
    const double x = tangentLength({m[0], m[1], m[2]}) * w / 2, y = tangentLength({m[4], m[5], m[6]}) * h / 2;
    const double scale = std::sqrt(x) * std::sqrt(y);
    if (std::isfinite(scale) && scale > 0)
        focal_ = scale;
}
std::optional<double> MatrixProjectionScale::at(domain::Vec3 p) const noexcept
{
    if (!domain::finite(p) || focal_ <= 0)
        return {};
    const double depth = depth_[0] * p.x + depth_[1] * p.y + depth_[2] * p.z + depth_[3];
    const double scale = focal_ / depth;
    return std::isfinite(depth) && depth > .001 && std::isfinite(scale) && scale > 0 ? std::optional(scale)
                                                                                     : std::nullopt;
}
std::optional<double> matrixPixelsPerWorld(const std::array<double, 16> &m, domain::Vec3 p, float w, float h) noexcept
{
    return MatrixProjectionScale(m, w, h).at(p);
}
} // namespace mousebox::render
