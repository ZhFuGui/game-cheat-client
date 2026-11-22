#pragma once
#include <compare>
#include <cstdint>

namespace mousebox::games::cs2
{
struct TargetKey
{
    std::uint32_t controller = 0, pawn = 0;
    auto operator<=>(const TargetKey &) const = default;
};
} // namespace mousebox::games::cs2
