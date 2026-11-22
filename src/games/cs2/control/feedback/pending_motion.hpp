#pragma once
#include "games/cs2/model/geometry.hpp"
#include <cstdint>
#include <deque>
namespace mousebox::games::cs2::feedback
{
// Predictive bookkeeping is separate from the fixed output-qualification rules.
class PendingMotion
{
    struct Entry
    {
        std::uint64_t at;
        double yaw, pitch;
    };
    std::deque<Entry> entries_;

  public:
    void clear()
    {
        entries_.clear();
    }
    std::size_t size() const noexcept
    {
        return entries_.size();
    }
    void retire(double sampleMs, double responseMs)
    {
        while (!entries_.empty() && static_cast<double>(entries_.front().at) + responseMs <= sampleMs)
            entries_.pop_front();
    }
    bool expired(std::uint64_t now) const noexcept
    {
        return entries_.size() >= 512 || (!entries_.empty() && now - entries_.front().at > 750);
    }
    double expectedAfter(double responseMs) const
    {
        return static_cast<double>(entries_.back().at) + responseMs;
    }
    Angles residual(Angles error, bool enabled = true) const
    {
        if (enabled)
            for (const auto &p : entries_)
            {
                error.y -= p.yaw;
                error.x -= p.pitch;
            }
        return error;
    }
    void add(std::uint64_t at, double yaw, double pitch)
    {
        entries_.push_back({at, yaw, pitch});
    }
    void rejectLast()
    {
        if (!entries_.empty())
            entries_.pop_back();
    }
};
} // namespace mousebox::games::cs2::feedback
