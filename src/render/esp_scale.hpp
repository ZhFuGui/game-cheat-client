#pragma once
#include "domain/math.hpp"
#include <array>
#include <cstddef>
#include <optional>

namespace mousebox::render
{
// A model-relative decoration size, never a measured hitbox or a meter conversion.
class SkeletonReference
{
    std::size_t count_ = 0;
    domain::Vec3 mean_;
    double variance_ = 0, longestEdge_ = 0, headSpan_ = 0;

  public:
    void point(domain::Vec3);
    void edge(domain::Vec3, domain::Vec3, bool headNeck = false);
    double span() const noexcept;
};
struct EspScale
{
    float jointRadius = 0, boneWidth = 0;
    float annotation = 1;
};
// pixelsPerWorld is the local camera-facing projection scale at this point.
EspScale espScale(double pixelsPerWorld, double referenceSpan) noexcept;
// Projection constants are prepared once per scene, not once for every joint.
class MatrixProjectionScale
{
    std::array<double, 4> depth_{};
    double focal_ = 0;

  public:
    MatrixProjectionScale(const std::array<double, 16> &, float width, float height) noexcept;
    std::optional<double> at(domain::Vec3) const noexcept;
};
// Row-major matrix with column vectors, matching CS2's existing projection.
std::optional<double> matrixPixelsPerWorld(const std::array<double, 16> &, domain::Vec3, float width,
                                           float height) noexcept;
} // namespace mousebox::render
