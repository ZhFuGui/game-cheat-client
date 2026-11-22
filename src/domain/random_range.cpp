#include "random_range.hpp"
#include <array>
#include <cmath>
#include <stdexcept>

namespace mousebox::random
{
namespace
{
constexpr std::array<std::string_view, 5> Ids{"normal", "uniform", "triangular", "lower", "upper"};
}

std::string_view distributionId(Distribution distribution) noexcept
{
    const auto index = static_cast<unsigned>(distribution);
    return index < Ids.size() ? Ids[index] : std::string_view{};
}

std::optional<Distribution> distributionFromId(std::string_view id) noexcept
{
    for (unsigned i = 0; i < Ids.size(); ++i)
        if (Ids[i] == id)
            return static_cast<Distribution>(i);
    return {};
}

unsigned IntegerRange::sample(std::mt19937 &generator) const
{
    if (min > max || distributionId(distribution).empty())
        throw std::invalid_argument("Invalid random range or distribution");
    if (min == max)
        return min;
    if (distribution == Distribution::Uniform)
        return std::uniform_int_distribution<unsigned>(min, max)(generator);

    const double width = static_cast<double>(max) - min;
    if (distribution == Distribution::Normal)
    {
        std::normal_distribution<double> gaussian(min + width / 2, width / 6);
        double value;
        do
        {
            value = gaussian(generator);
        } while (value < min || value > max);
        return static_cast<unsigned>(std::llround(value));
    }

    const auto unit = std::uniform_real_distribution<double>(0, 1)(generator);
    double position;
    switch (distribution)
    {
    case Distribution::Triangular:
        position = unit < .5 ? std::sqrt(unit / 2) : 1 - std::sqrt((1 - unit) / 2);
        break;
    case Distribution::Lower:
        position = 1 - std::sqrt(1 - unit);
        break;
    case Distribution::Upper:
        position = std::sqrt(unit);
        break;
    default:
        throw std::invalid_argument("Invalid random distribution");
    }
    return static_cast<unsigned>(std::llround(min + width * position));
}
} // namespace mousebox::random
