#include "runtime_service_impl.hpp"
#include "runtime_service_util.hpp"
#include "application/storage/human_store.hpp"
#include "games/registry.hpp"
#include <chrono>

namespace mousebox::application
{
using namespace runtime_detail;

Json RuntimeService::Impl::workspaceContext() const
{
    if (workspaceTransition || !workspaceAvailable || closed)
        return nullptr;
    const auto id = workspaceStore.activeId();
    return {{"workspaceId", id},
            {"gameId", workspaceStore.summary(id).at("gameId")},
            {"epoch", std::to_string(workspaceEpoch)}};
}
Json RuntimeService::workspaceContext() const
{
    const std::lock_guard lock(impl_->mutex);
    return impl_->workspaceContext();
}
void RuntimeService::requireWorkspaceContext(const Json &context) const
{
    const std::lock_guard lock(impl_->mutex);
    const auto current = impl_->workspaceContext();
    require(!current.is_null() && context.is_object() && context == current,
            "工作区上下文已过期或不匹配，请刷新当前游戏");
}
Json RuntimeService::Impl::workspaceCatalog() const
{
    auto gameList = Json::array();
    for (const auto &definition : games::gameDefinitions())
    {
        auto capabilities = Json::array();
        for (const auto capability : definition.capabilities)
            capabilities.push_back(capability);
        gameList.push_back({{"id", definition.id},
                            {"name", definition.name},
                            {"status", definition.status},
                            {"reason", definition.reason},
                            {"capabilities", std::move(capabilities)}});
    }
    return {{"schemaVersion", 1},
            {"active", workspaceContext()},
            {"workspaces", workspaceStore.index().at("workspaces")},
            {"games", std::move(gameList)}};
}
Json RuntimeService::Impl::motionBinding() const
{
    return {{"modelPath", storage::utf8(learningState.modelPath.wstring())},
            {"active", learningState.active},
            {"variation", learningState.variation},
            {"captureDpi", learningState.captureDpi}};
}
void RuntimeService::Impl::restoreMotionBinding()
{
    const auto binding = workspaceStore.motionBinding(workspaceStore.activeId());
    learningState.active = false;
    learningState.model.reset();
    learningState.modelPath.clear();
    learningState.variation = binding.value("variation", true);
    learningState.captureDpi = binding.value("captureDpi", values().at("dpi").get<unsigned>());
    require(learningState.captureDpi >= 100 && learningState.captureDpi <= 32000, "工作区采集DPI无效");
    const auto path = binding.value("modelPath", std::string{});
    if (path.empty())
    {
        applyHumanModel(nullptr, values().at("dpi").get<unsigned>(), false);
        return;
    }
    learningState.modelPath = workspaceStore.resolveAssetPath(storage::wide(path));
    try
    {
        if (humanModelFileKind(learningState.modelPath) == ModelFileKind::Speed)
            learningState.model =
                std::make_shared<const control::HumanMotionModel>(loadHumanModel(learningState.modelPath));
        learningState.active = binding.value("active", false) && static_cast<bool>(learningState.model);
        if (binding.value("active", false) && !learningState.active)
            learningState.status = "保存的模型不可用，当前游戏保持停用；原文件未修改";
    }
    catch (const std::exception &error)
    {
        learningState.status = "保存的模型未载入：" + std::string(error.what());
        log(learningState.status);
    }
    applyHumanModel(learningState.active ? learningState.model : nullptr, values().at("dpi").get<unsigned>(),
                    learningState.active && learningState.variation);
}
Json RuntimeService::Impl::handleWorkspace(std::string_view command, const Json &payload)
{
    if (command != "workspace.switch")
    {
        const std::lock_guard lock(mutex);
        require(!closed && !workspaceTransition, "工作区当前不能修改");
        if (command == "workspace.read")
            return workspaceCatalog();
        throw std::invalid_argument("不支持新建或重命名游戏配置");
    }

    const auto id = payload.at("id").get<std::string>();
    Json next;
    std::function<void()> pause;
    bool persistPrevious = false;
    {
        const std::lock_guard lock(mutex);
        require(!closed && !workspaceTransition, "游戏正在切换或已关闭");
        next = workspaceStore.settings(id); // Invalid documents cannot disrupt the existing session.
        persistPrevious = workspaceAvailable;
        transitionLogContext = workspaceContext();
        for (const auto &message : backend.takeUpdates().logs)
            runtimeLog.append(message, nowText(), transitionLogContext);
        workspaceTransition = true;
        workspaceAvailable = false;
        ++workspaceEpoch; // All former requests become stale before cleanup starts.
        pause = pauseCapture;
        fitWorker.request_stop();
        gameModule->setSessionEligible(false);
    }
    try
    {
        // CaptureClient::stop has a sequence barrier; it restores its own cursor
        // clip and delivers every accepted record before returning.
        if (pause)
            pause();
        {
            const std::lock_guard lock(mutex);
            acceptPendingLearning();
            updateMouseReports();
            learningState.recording = false;
            learningState.replaying = false;
            backend.setCapturing(false);
            saveDirtySamples();
        }
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        require(fileTasks.waitIdle(std::chrono::seconds(30)), "等待旧游戏文件任务超时，输出保持停止");
        while (fitRunning.load())
        {
            require(std::chrono::steady_clock::now() < deadline, "等待旧拟合任务取消超时，输出保持停止");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        if (fitWorker.joinable())
            fitWorker.join();
        const std::lock_guard lock(mutex);
        if (persistPrevious)
            persist();
        const bool previousHasApplication = gameModule->application() != nullptr;
        gameModule->resetWorkspace();
        if (previousHasApplication)
            backend.resetWorkspace();
        for (const auto &message : backend.takeUpdates().logs)
            runtimeLog.append(message, nowText(), transitionLogContext);
        transitionLogContext = {{"workspaceId", id},
                                {"gameId", workspaceStore.summary(id).at("gameId")},
                                {"epoch", std::to_string(workspaceEpoch)}};
        settings = std::move(next);
        const auto nextGame = workspaceStore.summary(id).at("gameId").get<std::string>();
        if (gameModule->definition().id != nextGame)
            installModule(nextGame);
        configureActive();
        workspaceStore.activate(id);
        algorithmSubscribed = false;
        calibrationSaved = false;
        learningState.training = false;
        learningState.fitFinished = false;
        learningState.fitted.reset();
        learningState.fitError.clear();
        invalidateDeletion();
        restoreMotionBinding();
        reloadCatalog();
        renderStats = {};
        workspaceTransition = false;
        workspaceAvailable = true;
        log("已切换游戏：" + workspaceStore.summary(id).at("name").get<std::string>());
        return workspaceCatalog();
    }
    catch (...)
    {
        const std::lock_guard lock(mutex);
        workspaceTransition = false;
        workspaceAvailable = false;
        gameModule->setSessionEligible(false);
        gameModule->resetWorkspace();
        // A failed index commit can happen after applying the candidate values.
        // Keep the visible data aligned to the still-committed index; shutdown
        // must never save a candidate under the preceding workspace identity.
        try
        {
            settings = workspaceStore.settings(workspaceStore.activeId());
            const auto committedGame =
                workspaceStore.summary(workspaceStore.activeId()).at("gameId").get<std::string>();
            if (gameModule->definition().id != committedGame)
                installModule(committedGame);
            configureActive();
            restoreMotionBinding();
        }
        catch (const std::exception &)
        {
            learningState.active = false;
            learningState.model.reset();
        }
        log("游戏切换失败，输出与数据源已停止；请选择有效游戏重试");
        throw;
    }
}
} // namespace mousebox::application
