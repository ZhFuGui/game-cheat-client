#include "connection_status.hpp"
namespace mousebox::application
{
namespace
{
Json indicator(const char *phase, std::string message)
{
    return {{"phase", phase}, {"message", std::move(message)}};
}
} // namespace
Json describeReaderConnection(const Snapshot &s, std::uint64_t now)
{
    if (!s.readerRunning)
        return indicator("closed", "未连接");
    if (!s.readerLivingError.empty() || !s.readerLocalError.empty())
        return indicator("error",
                         "连接失败 · " + (!s.readerLivingError.empty() ? s.readerLivingError : s.readerLocalError));
    if (s.livingConnected && s.viewConnected)
    {
        const auto fresh = [&](std::uint64_t observed) { return observed && now >= observed && now - observed <= 800; };
        if (!s.living || !s.view || !fresh(s.livingObservedMs) || !fresh(s.viewObservedMs) ||
            !control::checkPair(*s.living, *s.view, {s.sourceSkewMs, 800}).usable() || !s.living->self || !s.view->self)
            return indicator("degraded", "已连接，等待完整当前数据");
        return indicator("ready", "已连接");
    }
    if (s.livingConnected || s.viewConnected)
        return indicator("degraded", "部分数据流已连接，正在恢复");
    return indicator("connecting", "正在连接或重试");
}
} // namespace mousebox::application
