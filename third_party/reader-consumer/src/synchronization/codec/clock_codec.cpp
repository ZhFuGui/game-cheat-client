#include "synchronization/codec/detail.hpp"
#include <algorithm>
#include <limits>

namespace monitor::synchronization::codec {
namespace {
u64 Add(u64 a, u64 b) {
    detail::Require(b <= UINT64_MAX - a, "Clock arithmetic overflow", ErrorCode::InvalidClock);
    return a + b;
}
// 可移植128位乘积/64位除法，常见小差值直接走64位；不依赖Windows intrinsic或浮点时戳。
u64 MulDiv(u64 a, u64 b, u64 divisor, bool up) {
    detail::Require(divisor, "Zero clock divisor", ErrorCode::InvalidClock);
    if (!a || !b)
        return 0;
    if (a <= UINT64_MAX / b) {
        const u64 product = a * b;
        return Add(product / divisor, up && product % divisor ? 1 : 0);
    }
    constexpr u64 mask = 0xffffffffULL;
    const u64 a0 = a & mask, a1 = a >> 32, b0 = b & mask, b1 = b >> 32;
    const u64 w0 = a0 * b0, t = a1 * b0 + (w0 >> 32);
    const u64 w1 = (t & mask) + a0 * b1;
    const u64 high = a1 * b1 + (t >> 32) + (w1 >> 32), low = (w1 << 32) + (w0 & mask);
    detail::Require(high < divisor, "Clock quotient overflow", ErrorCode::InvalidClock);
    u64 remainder = high, result = 0;
    for (int bit = 63; bit >= 0; --bit) {
        const bool carry = (remainder >> 63) != 0;
        remainder = (remainder << 1) | ((low >> bit) & 1);
        if (carry || remainder >= divisor) {
            remainder -= divisor;
            result |= u64(1) << bit;
        }
    }
    return Add(result, up && remainder ? 1 : 0);
}
template <class F> bool Checked(F&& work, Error& error) {
    try {
        work();
        error = {};
        return true;
    } catch (const detail::Failure& fail) {
        error = {fail.code, fail.what()};
        return false;
    }
}
void Mapping(const ClockMapping& map) {
    detail::Require(!map.domain_id.empty() && map.domain_id.size() <= 128 && map.qpc_frequency &&
                        map.mapping_revision && map.reader_monotonic_anchor_ns && map.qpc_anchor &&
                        map.valid_from_qpc <= map.qpc_anchor && map.qpc_anchor < map.valid_until_qpc,
                    "Invalid clock mapping", ErrorCode::InvalidClock);
}
} // namespace
bool ValidateClock(const ClockMapping& map, Error& error) {
    return Checked(
        [&] {
            Mapping(map);
            detail::Require(map.max_error_ns <= 1000000, "Clock error exceeds accuracy bound", ErrorCode::InvalidClock);
            detail::Require(MulDiv(1, 1000000000, map.qpc_frequency, true) <= 1000000,
                            "Clock rounding exceeds accuracy bound", ErrorCode::InvalidClock);
        },
        error);
}
bool ValidateClockIdentity(const ClockIdentity& clock, Error& error) {
    return Checked(
        [&] {
            detail::Require(!clock.domain_id.empty() && clock.domain_id.size() <= 128 &&
                                std::all_of(clock.domain_id.begin(), clock.domain_id.end(),
                                            [](unsigned char c) { return c >= 33 && c <= 126; }) &&
                                (clock.kind == ClockKind::WindowsQpc || clock.kind == ClockKind::MonotonicCounter) &&
                                clock.frequency && clock.frequency <= 1000000000000ULL,
                            "Invalid consumer clock identity", ErrorCode::InvalidClock);
        },
        error);
}
bool ToQpc(const ClockMapping& map, Clock::time_point point, bool up, u64& output, Error& error) {
    u64 result = 0;
    const bool good = Checked(
        [&] {
            Mapping(map);
            const auto count = std::chrono::duration_cast<std::chrono::nanoseconds>(point.time_since_epoch()).count();
            detail::Require(count >= 0, "Negative clock sample", ErrorCode::InvalidClock);
            const u64 ns = u64(count);
            if (ns >= map.reader_monotonic_anchor_ns)
                result =
                    Add(map.qpc_anchor, MulDiv(ns - map.reader_monotonic_anchor_ns, map.qpc_frequency, 1000000000, up));
            else {
                const auto delta = MulDiv(map.reader_monotonic_anchor_ns - ns, map.qpc_frequency, 1000000000, !up);
                detail::Require(delta <= map.qpc_anchor, "Clock sample underflow", ErrorCode::InvalidClock);
                result = map.qpc_anchor - delta;
            }
            detail::Require(result >= map.valid_from_qpc && result <= map.valid_until_qpc,
                            "Sample outside clock window", ErrorCode::InvalidClock);
        },
        error);
    if (good)
        output = result;
    return good;
}
bool FromQpc(const ClockMapping& map, u64 qpc, Clock::time_point& output, Error& error) {
    Clock::time_point result;
    const bool good = Checked(
        [&] {
            Mapping(map);
            detail::Require(qpc >= map.valid_from_qpc && qpc <= map.valid_until_qpc, "Sample outside clock window",
                            ErrorCode::InvalidClock);
            u64 ns;
            if (qpc >= map.qpc_anchor)
                ns = Add(map.reader_monotonic_anchor_ns,
                         MulDiv(qpc - map.qpc_anchor, 1000000000, map.qpc_frequency, false));
            else {
                const auto delta = MulDiv(map.qpc_anchor - qpc, 1000000000, map.qpc_frequency, true);
                detail::Require(delta <= map.reader_monotonic_anchor_ns, "Monotonic sample underflow",
                                ErrorCode::InvalidClock);
                ns = map.reader_monotonic_anchor_ns - delta;
            }
            detail::Require(ns <= u64(INT64_MAX), "Monotonic sample overflow", ErrorCode::InvalidClock);
            result = Clock::time_point(std::chrono::duration_cast<Clock::duration>(std::chrono::nanoseconds(ns)));
        },
        error);
    if (good)
        output = result;
    return good;
}
bool ValidatePolicy(const TransportPolicy& p, Error& error) {
    return Checked(
        [&] {
            detail::Require(p.max_tcp_record_bytes >= 65536 && p.max_tcp_record_bytes <= 16777216 &&
                                p.heartbeat_interval_ms >= 100 && p.heartbeat_interval_ms <= 1000 &&
                                p.heartbeat_timeout_ms >= p.heartbeat_interval_ms * 3 &&
                                p.heartbeat_timeout_ms <= p.heartbeat_interval_ms * 10 &&
                                p.acknowledgement_timeout_ms >= 100 && p.acknowledgement_timeout_ms <= 3000 &&
                                p.record_progress_timeout_ms >= 100 && p.record_progress_timeout_ms <= 3000 &&
                                p.invalidation_delivery_timeout_ms >= 10 && p.invalidation_delivery_timeout_ms <= 250,
                            "Invalid transport policy");
        },
        error);
}
} // namespace monitor::synchronization::codec
