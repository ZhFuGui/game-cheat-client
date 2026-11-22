#pragma once
#include <cstdint>
#include <functional>

namespace mousebox::games::cs2
{
// Applies only to programs that already require foreground qualification.
// Disabling the OS window comparison does not invent a valid game identity.
struct ForegroundPolicy
{
    bool required = true;
    bool permits(std::uint32_t gameProcess, const std::function<std::uint32_t()> &foreground) const
    {
        return gameProcess != 0 && (!required || foreground() == gameProcess);
    }
    bool operator==(const ForegroundPolicy &) const = default;
};
} // namespace mousebox::games::cs2
