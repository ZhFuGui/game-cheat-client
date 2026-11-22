#pragma once
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
namespace monitor {
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i32 = std::int32_t;
using Clock = std::chrono::steady_clock;
struct ArrayHeader {
    u64 data = 0;
    i32 num = 0;
    i32 max = 0;
};
static_assert(sizeof(ArrayHeader) == 16 && offsetof(ArrayHeader, num) == 8 && offsetof(ArrayHeader, max) == 12,
              "Unexpected array layout");
inline bool operator==(const ArrayHeader& a, const ArrayHeader& b) noexcept {
    return a.data == b.data && a.num == b.num && a.max == b.max;
}
struct Vec3 {
    float x = 0, y = 0, z = 0;
};
struct WorldOrigin {
    i32 x = 0, y = 0, z = 0;
    bool operator==(const WorldOrigin&) const = default;
};
static_assert(sizeof(WorldOrigin) == 12);
struct ObjectName {
    std::optional<u32> raw_index, raw_number, index, number;
    std::string base, full;
    bool wide = false, verified = false;
};
enum class NameCaptureStatus : unsigned { NotAttempted = 1, Succeeded = 2, Failed = 3 };
static_assert(sizeof(Vec3) == 12, "This observed layout uses three floats");
bool ValidPosition(const Vec3& v) noexcept;
} // namespace monitor
