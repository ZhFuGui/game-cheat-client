#pragma once
#include <span>
#include <string_view>

namespace mousebox::games
{
// Catalog capability means the shipped module implements a page/operation.
// Current source, clock, calibration and device qualification remain independent.
struct GameDefinition
{
    std::string_view id, name, status, reason;
    std::span<const std::string_view> capabilities;
    std::string_view windowExecutable;
};
} // namespace mousebox::games
