#pragma once
#include <optional>
#include <random>
#include <string_view>

namespace mousebox::random
{
enum class Distribution
{
    Normal,
    Uniform,
    Triangular,
    Lower,
    Upper
};

std::string_view distributionId(Distribution) noexcept;
std::optional<Distribution> distributionFromId(std::string_view) noexcept;

// Stateless sampling. The caller owns the generator and decides when to draw.
// Nonuniform distributions are continuous on the closed interval, then rounded
// to integers. Uniform gives each integer, including both bounds, equal weight.
struct IntegerRange
{
    unsigned min, max;
    Distribution distribution = Distribution::Normal;
    bool operator==(const IntegerRange &) const = default;
    unsigned sample(std::mt19937 &) const;
};
} // namespace mousebox::random
