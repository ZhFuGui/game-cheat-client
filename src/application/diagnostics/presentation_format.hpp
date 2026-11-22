#pragma once
#include "application/json.hpp"
#include <iomanip>
#include <optional>
#include <sstream>

namespace mousebox::application::presentation
{
inline std::string number(double value, int precision = 2)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}
inline Json nullable(std::optional<double> value)
{
    return value ? Json(*value) : Json{};
}
} // namespace mousebox::application::presentation
