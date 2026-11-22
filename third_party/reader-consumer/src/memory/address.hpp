#pragma once
#include "memory/issue.hpp"
#include <limits>
namespace monitor {
inline bool TryAdd(u64 a, u64 b, u64& out) noexcept {
    if (a > (std::numeric_limits<u64>::max)() - b)
        return false;
    out = a + b;
    return true;
}
inline u64 Add(u64 a, u64 b, const char* stage) {
    u64 out = 0;
    if (!TryAdd(a, b, out))
        Fail(Code::InvalidAddress, stage, "Address addition overflow", a);
    return out;
}
inline bool UserAddress(u64 address) noexcept {
    // This project's original 48-bit Windows-user-VA heuristic. Not a mapping test.
    return address >= 0x10000ULL && address < 0x0000800000000000ULL;
}
inline bool UserRange(u64 address, std::size_t bytes) noexcept {
    if (bytes == 0)
        return true;
    u64 last = 0;
    return UserAddress(address) && TryAdd(address, static_cast<u64>(bytes - 1), last) && UserAddress(last);
}
} // namespace monitor
