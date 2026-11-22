#pragma once
#include "assets.hpp"
#include <array>
#include <optional>

namespace mousebox::games::cs2::hitboxes
{
enum class PointDistribution
{
    Uniform,
    Center,
    Outer,
    Middle
};

// Boxes shrink about their center. Rounded shapes keep their axis and shrink their radius.
Definition insetDefinition(Definition, double fraction) noexcept;
// Independent unit variates select an interior position in resource-local coordinates.
std::optional<Vec3> sampleInterior(const Definition &, PointDistribution, const std::array<double, 5> &) noexcept;
} // namespace mousebox::games::cs2::hitboxes
