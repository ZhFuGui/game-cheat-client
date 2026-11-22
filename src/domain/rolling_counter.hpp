#pragma once
#include <array>
#include <cstdint>
namespace mousebox
{
// Fixed-memory trailing-second counter, with one-ms boundary resolution.
// It bounds storage, never the number or frequency of events.
class RollingCounter
{
    struct Bucket
    {
        std::uint64_t time = 0, count = 0;
    };
    std::array<Bucket, 1000> buckets_{};

  public:
    void add(std::uint64_t now, std::uint64_t count = 1)
    {
        auto &b = buckets_[now % buckets_.size()];
        if (b.time != now)
            b = {now, 0};
        b.count += count;
    }
    std::uint64_t count(std::uint64_t now) const
    {
        std::uint64_t total = 0;
        for (const auto &b : buckets_)
            if (b.time <= now && now - b.time < 1000)
                total += b.count;
        return total;
    }
};
} // namespace mousebox
