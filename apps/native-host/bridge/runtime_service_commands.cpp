#include "runtime_service_impl.hpp"
#include "runtime_service_util.hpp"

namespace mousebox::application
{
using namespace runtime_detail;

Json RuntimeService::handle(std::string_view command, const Json &payload)
{
    auto &d = *impl_;
    require(payload.is_object(), "命令参数必须为对象");
    if (command.starts_with("workspace."))
        return d.handleWorkspace(command, payload);
    if (command == "settings.set" &&
        (payload.value("key", std::string{}) == "dpi" || payload.value("key", std::string{}) == "renderAdapter"))
    {
        const std::lock_guard lock(d.mutex);
        require(!d.closed && !d.workspaceTransition, "运行时已关闭或正在切换");
        if (!d.workspaceAvailable)
        {
            const auto key = payload.at("key").get<std::string>();
            if (key == "renderAdapter")
            {
                bool found = payload.at("value") == "auto";
                for (const auto &adapter : d.graphics)
                    found = found || adapter.at("value") == payload.at("value");
                require(found, "请选择已检测到的显卡");
            }
            d.workspaceStore.setApplicationSetting(key, payload.at("value"));
            d.values()[key] = payload.at("value");
            return ok();
        }
    }
    {
        const std::lock_guard lock(d.mutex);
        require(!d.workspaceTransition, "游戏正在切换，请稍后重试");
        require(d.workspaceAvailable || command == "bootstrap" || command == "runtime.snapshot" ||
                    command.starts_with("logs.") || command.starts_with("box.") || command == "ports.refresh" ||
                    command == "graphics.refresh" || command == "control.stop",
                "游戏切换失败，请重新选择有效游戏");
    }
    if (command == "bootstrap" || command == "runtime.snapshot")
        return snapshot();
    if (command == "reader.inspect")
    {
        require(payload.size() <= 2 && (!payload.contains("stream") || payload["stream"].is_string()) &&
                    (!payload.contains("after") || payload["after"].is_string()),
                "源数据请求无效");
        for (auto it = payload.begin(); it != payload.end(); ++it)
            require(it.key() == "stream" || it.key() == "after", "源数据请求包含未知字段");
        const auto stream = payload.value("stream", std::string{}), after = payload.value("after", std::string{});
        require(stream.size() <= 32 && after.size() <= 512, "源数据请求过长");
        const std::lock_guard lock(d.mutex);
        require(!d.closed && !d.workspaceTransition && d.gameModule, "当前游戏不可用");
        if (!d.sourceInspector)
            d.sourceInspector = std::make_unique<SourceInspector>();
        return d.sourceInspector->read(d.gameModule->dataSource().inspectSources(), stream, after);
    }
    if (command == "algorithm.read")
    {
        const auto s = snapshot(true);
        return {{"algorithm", s["algorithm"]}, {"algorithmSections", s["algorithmSections"]}};
    }
    if (command == "mouseReports.start")
    {
        poll();
        std::function<void()> start;
        {
            const std::lock_guard lock(d.mutex);
            require(!d.closed && !d.learningState.recording && !d.learningState.training && !d.ioBusy &&
                        !d.reports.analyzing && !d.reports.progress.recording,
                    "请先结束当前采集或后台任务");
            require(d.backend.sessionEligible(), "运行会话未获资格");
            require(!d.backend.inspectSnapshot().calibrating, "请先结束实测校准");
            start = d.startReportCapture;
            require(static_cast<bool>(start), "原始输入采集尚未初始化");
            d.reports.progress.recording = true;
            d.backend.armTest(false);
            d.backend.setCapturing(true);
        }
        try
        {
            start();
        }
        catch (...)
        {
            const std::lock_guard lock(d.mutex);
            d.reports.progress.recording = false;
            d.backend.setCapturing(false);
            throw;
        }
        {
            const std::lock_guard lock(d.mutex);
            ++d.reports.revision;
            d.reports.result = nullptr;
            d.reports.data.reset();
            d.reports.error.clear();
        }
        poll();
        return ok();
    }
    if (command == "mouseReports.stop")
    {
        std::function<void()> pause;
        {
            const std::lock_guard lock(d.mutex);
            if (d.reports.progress.recording)
                pause = d.pauseCapture;
        }
        if (pause)
            pause();
        poll();
        return ok();
    }
    if (command == "mouseReports.read")
    {
        poll();
        const std::lock_guard lock(d.mutex);
        return {{"result", d.reports.result}};
    }
    const bool mustPause = command == "learning.pause" || command == "learning.clear" || command == "learning.fit" ||
                           command == "learning.replay" || command == "samples.import" || command == "samples.export";
    if (mustPause)
    {
        std::function<void()> pause;
        {
            const std::lock_guard lock(d.mutex);
            pause = d.pauseCapture;
        }
        if (pause)
            pause();
        {
            const std::lock_guard lock(d.mutex);
            d.learningState.recording = false;
            d.learningState.replaying = false;
            d.backend.setCapturing(false);
        }
        poll();
    }
    if (command == "learning.start")
    {
        std::function<void(unsigned)> start;
        const auto dpi = payload.at("dpi").get<unsigned>();
        {
            const std::lock_guard lock(d.mutex);
            require(!d.closed && !d.learningState.training && !d.ioBusy && !d.reports.progress.recording &&
                        !d.reports.analyzing,
                    "当前不能采集");
            require(d.backend.sessionEligible(), "运行会话未获资格");
            require(dpi >= 100 && dpi <= 32000, "采集DPI不合法");
            std::size_t count = 0;
            for (const auto &t : d.learningState.trials)
                count += t.samples.size();
            require(d.learningState.trials.size() < 1000 && count < 1999999, "采样已满，请导出并清空");
            start = d.startCapture;
            require(static_cast<bool>(start), "原始输入采集尚未初始化");
            d.learningState.captureDpi = dpi;
            d.learningState.recording = true;
            d.backend.setCapturing(true);
        }
        try
        {
            start(dpi);
        }
        catch (...)
        {
            const std::lock_guard lock(d.mutex);
            d.learningState.recording = false;
            d.backend.setCapturing(false);
            throw;
        }
        return ok();
    }
    if (command == "learning.clear")
    {
        std::function<void()> clear;
        {
            const std::lock_guard lock(d.mutex);
            clear = d.clearCapture;
        }
        if (clear)
            clear();
    }
    const std::lock_guard lock(d.mutex);
    require(!d.closed, "运行时已关闭");
    if (command.starts_with("samples.") || command.starts_with("model.") || command.starts_with("learning."))
        require(!d.ioBusy, "请等待当前文件操作完成");
    if (command.starts_with("game."))
    {
        if (command.ends_with(".calibration.start"))
        {
            const auto state = d.backend.inspectOutput();
            require(d.backend.sessionEligible() && state.device.boxReady && state.device.triggerAvailable && !state.capturing &&
                        !state.device.testArmed && !state.calibrating && !d.learningState.training && !d.ioBusy,
                    "请先完成其他采集、测试或后台任务，并连接盒子回传");
        }
        if (auto app = d.gameModule->application())
            if (auto result = app->command(command, payload))
                return std::move(*result);
    }
    if (auto result = d.handleReportsLocked(command, payload))
        return std::move(*result);
    if (auto result = d.handleSettingsLocked(command, payload))
        return std::move(*result);
    if (auto result = d.handleConnectionsLocked(command, payload))
        return std::move(*result);
    if (auto result = d.handleModelsLocked(command, payload))
        return std::move(*result);
    if (auto result = d.handleLearningLocked(command, payload))
        return std::move(*result);
    if (auto result = d.handleLogsLocked(command, payload))
        return std::move(*result);
    throw std::invalid_argument("未知原生命令：" + std::string(command));
}
} // namespace mousebox::application
