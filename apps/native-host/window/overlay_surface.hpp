#pragma once
#include "game_window.hpp"
#include "application/json.hpp"
#include "render/directx_renderer.hpp"
#include <memory>
namespace mousebox::desktop
{
class OverlaySurface
{
    HWND window_ = nullptr;
    std::unique_ptr<render::DirectXRenderer> renderer_;
    render::SceneSubmission scene_;
    ForegroundGameWindow target_;
    std::string adapter_ = "auto", capture_ = "include", error_;
    RECT bounds_{};
    unsigned width_ = 0, height_ = 0;
    float scale_ = 1;
    bool visible_ = false, captureApplied_ = false;
    void prepare();
    void applyCapture();

  public:
    ~OverlaySurface();
    void configure(std::string capture);
    void adapter(std::string);
    void refreshAdapter();
    void poll(const application::ports::GameWindowTarget &, bool requested);
    void hide();
    void scene(render::SceneSubmission);
    bool visible() const noexcept
    {
        return visible_;
    }
    render::RenderStats stats() const;
    application::Json status() const;
    application::Json identity() const;
};
} // namespace mousebox::desktop
