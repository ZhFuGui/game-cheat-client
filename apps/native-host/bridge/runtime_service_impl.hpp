#pragma once
#include "runtime_service.hpp"
#include "application/tasks/background_queue.hpp"
#include "application/diagnostics/source_inspector.hpp"
#include "application/diagnostics/runtime_log.hpp"
#include "application/settings/settings.hpp"
#include "application/workspaces/workspace_store.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>

namespace mousebox::application
{
// The state mutex protects commands, UI snapshots and worker completions.
// inputMutex only protects the capture handoff; never acquire mutex while
// holding inputMutex. Raw Input callbacks never wait for file work.
struct RuntimeService::Impl
{
    RuntimeOptions options;
    Backend backend;
    WorkspaceStore workspaceStore;
    std::unique_ptr<ports::GameModule> gameModule;
    std::atomic<bool> separateApplication = false;
    std::function<void()> sceneNotifier;
    void installModule(std::string_view gameId);
    void configureActive();
    void applyHumanModel(std::shared_ptr<const control::HumanMotionModel>, unsigned dpi, bool variation);
    std::uint64_t workspaceEpoch = 1;
    bool workspaceTransition = false, workspaceAvailable = true, presentationActive = false;
    Json transitionLogContext = nullptr;
    std::atomic<bool> fitRunning = false;
    mutable std::mutex mutex;
    std::mutex inputMutex;
    std::vector<control::MouseTrial> pendingTrials;
    std::size_t storedTrialCount = 0, storedSampleCount = 0, pendingSampleCount = 0;
    struct CaptureState
    {
        bool recording;
        std::string status;
        double x, y;
    };
    std::optional<CaptureState> pendingCapture;
    struct ReportsState
    {
        // Pending progress + data are published atomically under inputMutex.
        std::optional<MouseReportsProgress> pendingProgress;
        std::shared_ptr<const MouseReportRecording> pendingData, data;
        MouseReportsProgress progress;
        Json result = nullptr;
        bool analyzing = false;
        std::uint64_t revision = 0;
        std::string error;
    } reports;
    std::function<void()> startReportCapture;
    Json reportState() const;
    Json settings = normalizeSettings(Json::object()), ports = Json::array(), graphics = Json::array();
    struct LearningState
    {
        std::vector<control::MouseTrial> trials;
        std::shared_ptr<const control::HumanMotionModel> model, fitted;
        std::filesystem::path modelPath, lastRecording, deletePath;
        std::string status = "每场自然移动15–20秒；至少3个独立场次，不要求瞄准能力。";
        std::string feedback, deleteToken, deleteDigest, fitError;
        bool recording = false, training = false, active = false, variation = true;
        bool dirtySamples = false, replaying = false, savingSamples = false, fitFinished = false;
        std::uint64_t replayStarted = 0, modelGeneration = 1, deleteGeneration = 0;
        unsigned captureDpi = 1600;
        double captureX = 0, captureY = 0;
    } learningState;
    bool closed = false, algorithmSubscribed = false, calibrationSaved = false;
    std::string ballisticsError, graphicsError;
    void invalidateDeletion();
    std::jthread fitWorker;
    BackgroundQueue fileTasks;
    std::unique_ptr<SourceInspector> sourceInspector;
    std::mutex portsWaitMutex;
    std::condition_variable_any portsWake;
    std::jthread portsWorker;
    bool portsChecked = false;
    std::string portsError;

    bool ioBusy = false;
    void enqueue(std::function<void()> task);
    void startIo();
    void finishIo(std::string error = {});

    std::function<void(unsigned)> startCapture;
    std::function<void()> pauseCapture, clearCapture;
    render::RenderStats renderStats;
    RuntimeLog runtimeLog;
    explicit Impl(RuntimeOptions o) : options(std::move(o)), workspaceStore(options.dataDirectory) {}
    Json &values()
    {
        return settings["values"];
    }
    void log(const std::string &message);
    void persist();
    Json workspaceCatalog() const;
    Json workspaceContext() const;
    Json handleWorkspace(std::string_view, const Json &);
    void restoreMotionBinding();
    Json motionBinding() const;
    void deactivate();
    void updateMouseReports();
    void acceptPendingLearning();
    void updateLearningReplay();
    void completeFit();
    void saveDirtySamples();
    Json learning() const;
    Json discoverPorts();
    Json refreshPorts();
    void startPorts();
    Json refreshGraphics();
    void reloadCatalog();
    std::optional<Json> handleSettingsLocked(std::string_view, const Json &);
    std::optional<Json> handleConnectionsLocked(std::string_view, const Json &);
    std::optional<Json> handleModelsLocked(std::string_view, const Json &);
    std::optional<Json> handleLearningLocked(std::string_view, const Json &);
    std::optional<Json> handleReportsLocked(std::string_view, const Json &);
    std::optional<Json> handleLogsLocked(std::string_view, const Json &);
};
} // namespace mousebox::application
