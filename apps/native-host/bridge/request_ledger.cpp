#include "request_ledger.hpp"
#include <stdexcept>

namespace mousebox::desktop
{
application::Json RequestLedger::dispatch(const Json &message, const Execute &execute)
{
    Json answer = {{"version", 1}, {"session", session_}, {"id", ""}};
    if (message.is_object() && message.contains("context"))
        answer["context"] = message["context"];
    try
    {
        if (message.at("version") != 1 || !message.at("id").is_string() || !message.at("session").is_string())
            throw std::invalid_argument("Invalid envelope");
        const auto candidate = message.at("session").get<std::string>();
        if (candidate.empty() || candidate.size() > 128)
            throw std::invalid_argument("Invalid session");
        if (session_.empty())
            session_ = candidate;
        else if (session_ != candidate)
            throw std::invalid_argument("Wrong session generation");
        answer["session"] = session_;
        const auto id = message.at("id").get<std::string>();
        if (id.empty() || id.size() > 64)
            throw std::invalid_argument("Invalid request id");
        answer["id"] = id;
        const auto command = message.at("command").get<std::string>();
        if (command.size() > 128)
            throw std::invalid_argument("Command name too long");
        const auto cached = replies_.find(id);
        if (cached != replies_.end())
        {
            if (cached->second.first != message)
                throw std::invalid_argument("Request id reused with different data");
            return cached->second.second;
        }
        answer["result"] = execute(command, message.value("payload", Json::object()));
        answer["ok"] = true;
    }
    catch (const std::exception &error)
    {
        answer["ok"] = false;
        answer["error"] = error.what();
    }
    const auto bytes = answer.dump().size() + message.dump().size();
    if (!answer["id"].get<std::string>().empty() && bytes < 2 * 1024 * 1024)
    {
        if (replies_.size() >= 64 || replyBytes_ + bytes > 8 * 1024 * 1024)
        {
            replies_.clear();
            replyBytes_ = 0;
        }
        replies_.emplace(answer["id"].get<std::string>(), std::make_pair(message, answer));
        replyBytes_ += bytes;
    }
    return answer;
}
} // namespace mousebox::desktop
