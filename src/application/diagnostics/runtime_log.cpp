#include "runtime_log.hpp"
#include <algorithm>
#include <stdexcept>

namespace mousebox::application
{
void RuntimeLog::append(const std::string &message, const std::string &time, const Json &workspace)
{
    entries_.push_back({{"id", std::to_string(++cursor_)}, {"time", time}, {"level", "info"}, {"message", message}});
    if (workspace.is_object())
        entries_.back()["workspace"] = workspace;
    while (entries_.size() > Capacity)
        entries_.pop_front();
}

Json RuntimeLog::read(std::uint64_t after, unsigned limit) const
{
    if (limit == 0 || limit > 5000)
        throw std::runtime_error("日志数量超限");
    Json result = Json::array();
    for (const auto &row : entries_)
    {
        if (std::stoull(row.at("id").get<std::string>()) > after)
        {
            result.push_back(row);
            if (result.size() >= limit)
                break;
        }
    }
    const auto cursor =
        result.empty() ? std::to_string(std::min(after, cursor_)) : result.back()["id"].get<std::string>();
    return {{"entries", std::move(result)}, {"cursor", cursor}};
}

Json RuntimeLog::entries() const
{
    Json result = Json::array();
    for (const auto &row : entries_)
        result.push_back(row);
    return result;
}
} // namespace mousebox::application
