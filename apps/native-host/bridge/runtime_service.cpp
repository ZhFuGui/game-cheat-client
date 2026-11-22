#include "runtime_service_impl.hpp"
#include "runtime_service_util.hpp"
#include "application/storage/human_store.hpp"
#include "application/assets/asset_paths.hpp"
#include <algorithm>

namespace mousebox::application
{
using namespace runtime_detail;

void RuntimeService::Impl::enqueue(std::function<void()> task)
{
    fileTasks.enqueue(std::move(task));
}

void RuntimeService::Impl::startIo()
{
    fileTasks.start([this](const std::string &error) {
        const std::lock_guard lock(mutex);
        learningState.savingSamples = false;
        finishIo(error);
    });
}

void RuntimeService::Impl::finishIo(std::string error)
{
    ioBusy = false;
    if (!error.empty())
    {
        learningState.feedback = "文件操作失败：" + error;
        learningState.status = learningState.feedback;
        log(learningState.feedback);
    }
}

void RuntimeService::Impl::log(const std::string &message)
{
    auto context = workspaceContext();
    if (context.is_null())
        context = transitionLogContext;
    runtimeLog.append(message, nowText(), context);
}

RuntimeService::RuntimeService(RuntimeOptions options) : impl_(std::make_unique<Impl>(std::move(options)))
{
    auto &d = *impl_;
    require(!d.options.dataDirectory.empty(), "配置目录未指定");
    std::filesystem::create_directories(d.options.dataDirectory);
    d.workspaceStore.open(d.options.importedSettings);
    d.settings = d.workspaceStore.settings(d.workspaceStore.activeId());
    d.installModule(d.workspaceStore.summary(d.workspaceStore.activeId()).at("gameId").get<std::string>());
    d.configureActive();
    d.learningState.captureDpi = d.values()["dpi"].get<unsigned>();
    d.reloadCatalog();
    d.refreshGraphics();
    d.restoreMotionBinding();
    const auto legacy = std::filesystem::current_path() / L"human-model.json";
    try
    {
        if (d.learningState.modelPath.empty() && std::filesystem::exists(legacy) &&
            humanModelFileKind(legacy) == ModelFileKind::LegacyPointing)
            d.learningState.modelPath = legacy;
    }
    catch (const std::exception &)
    {
    }
    d.persist();
    d.startIo();
    d.startPorts();
}

RuntimeService::~RuntimeService()
{
    shutdown();
}

Backend &RuntimeService::backend()
{
    return impl_->backend;
}

Json RuntimeService::values() const
{
    const std::lock_guard lock(impl_->mutex);
    return impl_->settings["values"];
}

void RuntimeService::setSessionEligible(bool eligible)
{
    {
        const std::lock_guard lock(impl_->mutex);
        if (eligible && (!impl_->workspaceAvailable || impl_->workspaceTransition || impl_->closed))
            throw std::runtime_error("当前游戏尚未就绪，不能授予输出资格");
        impl_->gameModule->setSessionEligible(eligible);
    }
    if (!eligible)
    {
        std::function<void()> pause;
        {
            const std::lock_guard lock(impl_->mutex);
            pause = impl_->pauseCapture;
        }
        if (pause)
            pause();
    }
}

void RuntimeService::poll()
{
    auto &d = *impl_;
    const auto s = d.backend.takeUpdates();
    const std::lock_guard lock(d.mutex);
    if (d.closed)
        return;
    d.updateMouseReports();
    d.acceptPendingLearning();
    d.updateLearningReplay();
    for (const auto &line : s.logs)
        d.log(line);
    if (s.calibrationFinished && !d.calibrationSaved)
    {
        d.calibrationSaved = true;
        d.persist();
    }
    d.completeFit();
    d.saveDirtySamples();
}

void RuntimeService::shutdown()
{
    if (!impl_)
        return;
    auto &d = *impl_;
    std::function<void()> pause;
    {
        const std::lock_guard lock(d.mutex);
        if (d.closed)
            return;
        pause = d.pauseCapture;
    }
    d.gameModule->setSessionEligible(false);
    try
    {
        if (pause)
            pause();
    }
    catch (const std::exception &)
    {
    }
    try
    {
        poll();
    }
    catch (const std::exception &)
    {
    }
    {
        const std::lock_guard lock(d.mutex);
        if (d.closed)
            return;
        d.closed = true;
        d.learningState.recording = false;
        d.fitWorker.request_stop();
    }
    d.portsWorker.request_stop();
    d.portsWake.notify_all();
    if (d.portsWorker.joinable())
        d.portsWorker.join();
    if (d.fitWorker.joinable())
        d.fitWorker.join();
    d.fileTasks.drainAndStop();
    d.backend.disconnectBox();
    d.gameModule->dataSource().disconnect();
    const std::lock_guard lock(d.mutex);
    try
    {
        if (d.learningState.dirtySamples && !d.learningState.trials.empty())
        {
            if (d.learningState.lastRecording.empty())
                d.learningState.lastRecording = assets::directory(d.options.dataDirectory, assets::Kind::MouseCapture) /
                                                storage::wide("speed-capture-" + unique() + ".json");
            if (!saveMouseTrials(d.learningState.lastRecording, d.learningState.trials))
                d.learningState.status = "退出时采样保存失败，已有文件保留";
        }
    }
    catch (const std::exception &)
    {
        d.learningState.status = "退出时采样保存失败，已有文件保留";
    }
    try
    {
        d.persist();
    }
    catch (const std::exception &)
    {
        d.learningState.status = "退出时设置保存失败，已有文件保留";
    }
}
} // namespace mousebox::application
