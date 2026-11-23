#pragma once
#include "types.hpp"
#include <cstdint>
#include <deque>

namespace mousebox::control::movement
{
// Outstanding angular commands under an explicit response-time model, not device acknowledgements.
class Pending
{
    struct Entry
    {
        std::uint64_t token, at;
        Vector degrees;
    };
    std::deque<Entry> entries_;

  public:
    void clear() noexcept
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
    Vector residual(Vector value) const noexcept
    {
        for (const auto &entry : entries_)
        {
            value.x -= entry.degrees.x;
            value.y -= entry.degrees.y;
        }
        return value;
    }
    Vector residualAt(Vector value, double sampleMs, double responseMs, std::uint64_t excludedToken) const noexcept
    {
        for (const auto &entry : entries_)
            if (entry.token != excludedToken && static_cast<double>(entry.at) + responseMs > sampleMs)
            {
                value.x -= entry.degrees.x;
                value.y -= entry.degrees.y;
            }
        return value;
    }
    bool expired(std::uint64_t now) const noexcept
    {
        return entries_.size() >= 512 ||
               (!entries_.empty() && (now < entries_.front().at || now - entries_.front().at > 750));
    }
    void add(std::uint64_t token, std::uint64_t now, Vector degrees)
    {
        entries_.push_back({token, now, degrees});
    }
    void reject(std::uint64_t token) noexcept
    {
        if (!entries_.empty() && entries_.back().token == token)
            entries_.pop_back();
    }
};
} // namespace mousebox::control::movement
