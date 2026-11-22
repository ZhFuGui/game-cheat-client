#pragma once
#include "application/json.hpp"
#include "render/directx_renderer.hpp"
#include <Windows.h>
#include <memory>
#include <filesystem>
namespace mousebox::desktop
{
class NativeSurface
{
    HWND parent_ = nullptr, child_ = nullptr;
    DWORD parentPid_ = 0;
    std::unique_ptr<render::DirectXRenderer> renderer_;
    unsigned width_ = 0, height_ = 0;
    int x_ = 0, y_ = 0;
    float scale_ = 1;
    bool visible_ = false;
    std::string requestedAdapter_ = "auto";
    render::SceneSubmission scene_;

  public:
    explicit NativeSurface(DWORD parentPid) : parentPid_(parentPid)
    {
    }
    ~NativeSurface();
    void update(const application::Json &);
    void hide();
    bool visible() const noexcept
    {
        return visible_;
    }
    void scene(render::SceneSubmission scene);
    void setAdapter(std::string);
    void refreshAdapter();
    render::RenderStats stats() const;
    application::Json identity() const;
    void requestCapture();
    application::Json writeCapture(const std::filesystem::path &) const;
};
} // namespace mousebox::desktop
