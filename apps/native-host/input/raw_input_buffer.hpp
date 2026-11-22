#pragma once
#include <Windows.h>
#include <array>
#include <cstddef>
#include <functional>
#include <span>

namespace mousebox::desktop
{
// Only complete OS-delivered mouse records reach the consumer. No resampling or
// synthetic timestamps are added when several records arrive in one batch.
void visitRawMouseBuffer(std::span<const std::byte>, UINT records, const std::function<void(const RAWINPUT &)> &);
class RawInputBuffer
{
    alignas(8) std::array<std::byte, 64 * 1024> bytes_{};

  public:
    void drain(const std::function<void(const RAWINPUT &)> &);
};
} // namespace mousebox::desktop
