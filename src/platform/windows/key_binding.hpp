#pragma once
#include <optional>
#include <string_view>
namespace mousebox
{
// Stable DOM physical-key IDs are serialized; Win32 codes stay at this boundary.
std::optional<unsigned> localKeyCode(std::string_view) noexcept;
} // namespace mousebox
