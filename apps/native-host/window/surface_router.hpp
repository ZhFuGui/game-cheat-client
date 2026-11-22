#pragma once
#include "native_surface.hpp"
#include "overlay_surface.hpp"
#include "scene_completion.hpp"
namespace mousebox::desktop
{
// Owns output placement; both targets consume the same immutable scene API.
// The game process is only queried for its top-level client bounds.
class SurfaceRouter
{
    DWORD parentPid_;
    HWND parent_ = nullptr;
    NativeSurface embedded_;
    OverlaySurface overlay_;
    application::ports::GameWindowTarget target_;
    render::SceneSubmission scene_;
    DrawCompletion completed_;
    std::string placement_ = "embedded", capture_ = "include", renderer_ = "default", adapter_ = "auto";
    bool requested_ = false;
    void applyAdapter();
    void submitScene();

  public:
    explicit SurfaceRouter(DWORD parent, DrawCompletion completed = {})
        : parentPid_(parent), embedded_(parent), completed_(std::move(completed))
    {
    }
    void configure(const application::Json &);
    void update(const application::Json &);
    void target(application::ports::GameWindowTarget);
    bool poll();
    void hide();
    void minimize();
    bool visible() const noexcept
    {
        return overlay() ? overlay_.visible() : embedded_.visible();
    }
    bool overlay() const noexcept
    {
        return placement_ == "overlay" && !target_.executable.empty();
    }
    void scene(application::ports::PresentationFrame);
    void setAdapter(std::string);
    void refreshAdapter();
    render::RenderStats stats() const;
    application::Json identity() const;
    application::Json status() const;
    void requestCapture();
    application::Json writeCapture(const std::filesystem::path &) const;
};
} // namespace mousebox::desktop
