#pragma once
#include <cstdint>
#include <string_view>
namespace mousebox
{
std::uint32_t foregroundProcessId() noexcept;
bool foregroundExecutable(std::string_view filename) noexcept;
} // namespace mousebox
