#pragma once
#include "application/json.hpp"
#include "application/ports/game_module.hpp"
#include "games/pubg/runtime/backend.hpp"
#include "render/esp_options.hpp"
#include "render/directx_renderer.hpp"
#include "application/diagnostics/mouse_reports.hpp"
#include <filesystem>
#include <functional>
#include <memory>
namespace mousebox::application
{
struct RuntimeOptions
{
    std::filesystem::path dataDirectory;
    std::filesystem::path executableDirectory;
    std::optional<Json> importedSettings;
    std::function<std::unique_ptr<ports::GameModule>(std::string_view, Backend &)> createGameModule;
};
class RuntimeService
{
    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    explicit RuntimeService(RuntimeOptions);
    ~RuntimeService();
    RuntimeService(const RuntimeService &) = delete;
    RuntimeService &operator=(const RuntimeService &) = delete;
    Json handle(std::string_view command, const Json &payload = Json::object());
    Json snapshot(bool includeDiagnostics = false);
    void poll();
    void shutdown();
    Backend &backend();
    render::EspOptions espOptions() const;
    ports::PresentationFrame presentation(const ports::PresentationOptions &);
    ports::GameWindowTarget presentationTarget() const;
    void setPresentationActive(bool);
    Json values() const;
    Json workspaceContext() const;
    void requireWorkspaceContext(const Json &) const;
    void setSessionEligible(bool);
    void setCaptureHooks(std::function<void(unsigned)> start, std::function<void()> pause,
                         std::function<void()> clear = {});
    bool appendCapturedTrials(std::vector<control::MouseTrial>);
    void acceptCapturedTrials(std::vector<control::MouseTrial> trials, bool recording = false);
    void setCaptureStatus(bool recording, std::string status, double cumulativeX = 0, double cumulativeY = 0);
    void setReportCaptureHook(std::function<void()> start);
    void setReportCaptureStatus(MouseReportsProgress);
    void acceptMouseReports(MouseReportRecording);
    void setRenderStats(render::RenderStats);
    void setSceneNotifier(std::function<void()>);
    void drawCompleted(std::span<const domain::SourceSample>, double completedAtMs);
};
} // namespace mousebox::application
