#pragma once
#include "domain/math.hpp"
#include "primitives.hpp"

namespace mousebox::render
{
struct CameraBasis
{
    domain::Vec3 position, forward, right, up;
};
// CPU projection only. Basis, lens interpretation and source qualification belong to the caller.
class CameraProjection
{
    CameraBasis camera_;
    EspRect rect_;
    double scale_ = 0;
    bool perspective_ = true;
    domain::Vec3 local(domain::Vec3) const;
    std::optional<Point> screen(domain::Vec3) const;

  public:
    static std::optional<CameraProjection> create(CameraBasis, EspRect, double fovDegrees, EspFovAxis,
                                                  std::optional<double> orthographicWidth = {});
    std::optional<Point> point(domain::Vec3) const;
    std::optional<Line> segment(domain::Vec3, domain::Vec3) const;
};
} // namespace mousebox::render
