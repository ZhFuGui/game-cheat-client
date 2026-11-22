#pragma once

#include "application/json.hpp"
#include <cstdint>
#include <deque>
#include <string>

namespace mousebox::application
{
// Serialized by the runtime state mutex. Clearing entries deliberately keeps
// the monotonically increasing cursor valid for existing UI subscriptions.
class RuntimeLog
{
  public:
    void append(const std::string &message, const std::string &time, const Json &workspace = nullptr);
    Json read(std::uint64_t after, unsigned limit) const;
    Json entries() const;
    void clear()
    {
        entries_.clear();
    }

  private:
    static constexpr std::size_t Capacity = 4000;
    std::deque<Json> entries_;
    std::uint64_t cursor_ = 0;
};
} // namespace mousebox::application
