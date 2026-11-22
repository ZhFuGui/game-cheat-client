#include "runtime_service_impl.hpp"
#include "runtime_service_util.hpp"

namespace mousebox::application
{
using namespace runtime_detail;

std::optional<Json> RuntimeService::Impl::handleLogsLocked(std::string_view command, const Json &payload)
{
    auto &d = *this;
    if (command == "logs.read")
    {
        std::uint64_t after = 0;
        const auto token = payload.value("after", std::string{});
        if (!token.empty())
        {
            std::size_t used = 0;
            after = std::stoull(token, &used);
            require(used == token.size(), "日志游标不合法");
        }
        return d.runtimeLog.read(after, payload.value("limit", 500u));
    }
    if (command == "logs.clear")
    {
        d.runtimeLog.clear();
        return ok();
    }
    if (command == "logs.export")
    {
        require(!d.ioBusy, "请等待当前文件操作完成");
        const auto path = pathOf(payload);
        Json entries = d.runtimeLog.entries();
        d.ioBusy = true;
        d.enqueue([&d, path, entries = std::move(entries)] {
            try
            {
                require(storage::save(path, {{"version", 1}, {"entries", entries}}), "日志保存失败");
                const std::lock_guard done(d.mutex);
                d.learningState.status = "日志已导出";
                d.finishIo();
            }
            catch (const std::exception &e)
            {
                const std::lock_guard done(d.mutex);
                d.finishIo(e.what());
            }
        });
        return ok();
    }
    return std::nullopt;
}
} // namespace mousebox::application
