#include "runtime_service_impl.hpp"
#include "runtime_service_util.hpp"
#include "games/pubg/data/ballistic_profiles.hpp"
#include "domain/connection_policy.hpp"
#include "application/settings/game_configurations.hpp"

namespace mousebox::application
{
using namespace runtime_detail;

Json RuntimeService::Impl::discoverPorts()
{
    Json list = Json::array();
    for (const auto &p : backend.ports())
    {
        const auto name = storage::utf8(p.name);
        list.push_back({{"port", name}, {"label", name + " · " + storage::utf8(p.description)}});
    }
    return list;
}

Json RuntimeService::Impl::refreshPorts()
{
    try
    {
        ports = discoverPorts();
        portsError.clear();
    }
    catch (const std::exception &error)
    {
        ports = Json::array();
        portsError = error.what();
    }
    portsChecked = true;
    return ports;
}

void RuntimeService::Impl::startPorts()
{
    portsWorker = std::jthread([this](std::stop_token stop) {
        while (!stop.stop_requested())
        {
            Json next = Json::array();
            std::string error;
            try
            {
                next = discoverPorts();
            }
            catch (const std::exception &e)
            {
                error = e.what();
            }
            {
                const std::lock_guard lock(mutex);
                if (closed)
                    break;
                ports = std::move(next);
                portsError = std::move(error);
                portsChecked = true;
            }
            std::unique_lock wait(portsWaitMutex);
            portsWake.wait_for(wait, stop, std::chrono::milliseconds(AutomaticConnectionIntervalMs),
                               [] { return false; });
        }
    });
}

Json RuntimeService::Impl::refreshGraphics()
{
    try
    {
        Json list = Json::array({{{"value", "auto"}, {"label", "自动（高性能显卡优先）"}, {"automatic", true}}});
        bool found = values()["renderAdapter"] == "auto";
        for (const auto &a : render::enumerateRenderAdapters())
        {
            if (values()["renderAdapter"] == a.id)
                found = true;
            list.push_back({{"value", a.id},
                            {"label", storage::utf8(a.name)},
                            {"detail", "独立显存 " + number(a.dedicatedBytes / 1073741824.0, 2) + " GB · 共享上限 " +
                                           number(a.sharedBytes / 1073741824.0, 2) + " GB"},
                            {"automatic", false},
                            {"software", a.software}});
        }
        if (!found)
            list.push_back(
                {{"value", values()["renderAdapter"]}, {"label", "已保存的显卡暂不可用"}, {"automatic", false}});
        graphics = std::move(list);
        graphicsError.clear();
    }
    catch (const std::exception &e)
    {
        graphicsError = e.what();
    }
    return graphics;
}

void RuntimeService::Impl::reloadCatalog()
{
    if (gameModule->application())
    {
        ballisticsError.clear();
        return;
    }
    try
    {
        auto catalog = defaultBallisticProfiles(options.executableDirectory);
        const auto current = backend.motionConfiguration().ballisticCatalog;
        if (!current || current->revision != catalog->revision || current->sourcePath != catalog->sourcePath)
            backend.setBallisticCatalog(std::move(catalog));
        ballisticsError.clear();
    }
    catch (const std::exception &e)
    {
        ballisticsError = e.what();
        log("弹道资料载入失败：" + ballisticsError);
    }
}

std::optional<Json> RuntimeService::Impl::handleConnectionsLocked(std::string_view command, const Json &payload)
{
    auto &d = *this;
    if (command == "reader.connect")
    {
        const auto host = payload.value("host", d.values()["host"].get<std::string>());
        const auto port = payload.value("port", d.values()["port"].get<unsigned>());
        const auto hz = payload.value("hz", d.values()["hz"].get<double>());
        const auto &configuration = gameConfiguration(gameModule->definition().id);
        configuration.validateValue("host", host);
        configuration.validateValue("port", port);
        configuration.validateValue("hz", hz);
        d.gameModule->dataSource().connect({host, static_cast<std::uint16_t>(port), hz});
        return ok();
    }
    if (command == "reader.disconnect")
    {
        d.gameModule->dataSource().disconnect();
        return ok();
    }
    if (command == "box.connect")
    {
        const auto port = payload.at("port").get<std::string>();
        require(!port.empty() && port.size() <= 256, "串口名称不合法");
        d.backend.connectBox(storage::wide(port));
        return ok();
    }
    if (command == "box.disconnect")
    {
        d.backend.disconnectBox();
        return ok();
    }
    if (command == "ports.refresh")
        return d.refreshPorts();
    if (command == "graphics.refresh")
        return d.refreshGraphics();
    if (command == "control.stop")
    {
        d.gameModule->stopOutput();
        return ok();
    }
    if (command == "test.setArmed")
    {
        d.backend.armTest(payload.at("armed").get<bool>());
        return ok();
    }
    if (command == "recording.load")
    {
        require(!d.gameModule->application(), "当前游戏尚未提供专用证据回放入口，不能按其他游戏解析");
        require(!d.ioBusy, "请等待当前文件操作完成");
        const auto path = pathOf(payload);
        d.ioBusy = true;
        d.learningState.status = "正在载入录制证据…";
        d.enqueue([&d, path] {
            try
            {
                d.backend.loadRecording(path);
                const std::lock_guard done(d.mutex);
                d.learningState.status = "已载入协议核验录制；离线预览不输出鼠标";
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
    if (command == "algorithm.subscribe")
    {
        d.algorithmSubscribed = payload.at("enabled").get<bool>();
        return ok();
    }
    if (command == "ballistics.reload")
    {
        d.reloadCatalog();
        return ok();
    }
    return std::nullopt;
}
} // namespace mousebox::application
