#pragma once
#include "application/storage/json_store.hpp"
#include <functional>
#include <map>
#include <string>

namespace mousebox::desktop
{
// Single host-message-thread owner. Transport and command handlers do not own
// session identity or retry policy; duplicate commands cannot repeat side effects within the retained reply window.
class RequestLedger
{
    using Json = application::Json;
    std::string session_;
    std::map<std::string, std::pair<Json, Json>> replies_;
    std::size_t replyBytes_ = 0;

  public:
    using Execute = std::function<Json(const std::string &, const Json &)>;
    const std::string &session() const noexcept
    {
        return session_;
    }
    Json dispatch(const Json &message, const Execute &execute);
};
} // namespace mousebox::desktop
