#include "overlay_surface.hpp"
#include "resources/embedded_font.hpp"
#include "application/storage/json_store.hpp"
#include <winternl.h>
#include <stdexcept>
namespace mousebox::desktop
{
namespace
{
LRESULT CALLBACK overlayProcedure(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    if (message == WM_NCHITTEST)
        return HTTRANSPARENT;
    if (message == WM_MOUSEACTIVATE)
        return MA_NOACTIVATE;
    if (message == WM_ERASEBKGND)
        return 1;
    if (message == WM_DPICHANGED)
        return 0;
    if (message == WM_PAINT)
    {
        PAINTSTRUCT paint{};
        BeginPaint(window, &paint);
        EndPaint(window, &paint);
        return 0;
    }
    return DefWindowProcW(window, message, wp, lp);
}
bool supportsCaptureExclusion()
{
    using Version = LONG(WINAPI *)(PRTL_OSVERSIONINFOW);
    const auto fn = reinterpret_cast<Version>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
    RTL_OSVERSIONINFOW version{sizeof(version)};
    return fn && fn(&version) == 0 &&
           (version.dwMajorVersion > 10 || (version.dwMajorVersion == 10 && version.dwBuildNumber >= 19041));
}
} // namespace
OverlaySurface::~OverlaySurface()
{
    renderer_.reset();
    if (window_)
        DestroyWindow(window_);
}
void OverlaySurface::prepare()
{
    if (window_)
        return;
    WNDCLASSW cls{};
    cls.lpfnWndProc = overlayProcedure;
    cls.hInstance = GetModuleHandleW(nullptr);
    cls.lpszClassName = L"MouseboxGameOverlay";
    RegisterClassW(&cls);
    window_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT | WS_EX_LAYERED |
                                  WS_EX_NOREDIRECTIONBITMAP,
                              cls.lpszClassName, L"Mousebox ESP", WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, cls.hInstance,
                              nullptr);
    if (!window_)
        throw std::runtime_error("无法创建游戏叠加层");
    if (!SetLayeredWindowAttributes(window_, 0, 255, LWA_ALPHA))
        throw std::runtime_error("无法初始化叠加层透明度");
}
void OverlaySurface::applyCapture()
{
    captureApplied_ = false;
    if (capture_ == "exclude" && !supportsCaptureExclusion())
    {
        error_ = "当前 Windows 不支持排除叠加层截图";
        return;
    }
    constexpr DWORD exclude = 0x11;
    const DWORD wanted = capture_ == "exclude" ? exclude : WDA_NONE;
    DWORD applied = 0;
    if (!SetWindowDisplayAffinity(window_, wanted) || !GetWindowDisplayAffinity(window_, &applied) || applied != wanted)
    {
        error_ = "系统未能应用叠加层截图策略";
        return;
    }
    captureApplied_ = true;
    error_.clear();
}
void OverlaySurface::configure(std::string capture)
{
    if (capture != "include" && capture != "exclude")
        throw std::invalid_argument("Invalid overlay capture policy");
    capture_ = std::move(capture);
    try
    {
        prepare();
        applyCapture();
    }
    catch (const std::exception &e)
    {
        error_ = e.what();
        captureApplied_ = false;
    }
    if (!captureApplied_)
        hide();
}
void OverlaySurface::adapter(std::string id)
{
    if (adapter_ == id)
        return;
    adapter_ = std::move(id);
    if (renderer_)
        renderer_->setAdapter(adapter_);
}
void OverlaySurface::refreshAdapter()
{
    if (renderer_)
        renderer_->recreateDevice();
}
void OverlaySurface::hide()
{
    if (window_)
        ShowWindow(window_, SW_HIDE);
    if (visible_ && renderer_)
        renderer_->submit(scene_, width_, height_, scale_, false);
    visible_ = false;
}
void OverlaySurface::poll(const application::ports::GameWindowTarget &expected, bool requested)
{
    if (!requested || expected.executable.empty() || !captureApplied_)
    {
        hide();
        return;
    }
    const auto found = target_.sample(expected);
    if (!found)
    {
        hide();
        return;
    }
    try
    {
        prepare();
        const auto &r = found->client;
        const auto width = static_cast<unsigned>(r.right - r.left), height = static_cast<unsigned>(r.bottom - r.top);
        const bool moved =
            r.left != bounds_.left || r.top != bounds_.top || r.right != bounds_.right || r.bottom != bounds_.bottom;
        if (moved || !visible_)
            if (!SetWindowPos(window_, HWND_TOPMOST, r.left, r.top, static_cast<int>(width), static_cast<int>(height),
                              SWP_NOACTIVATE))
                throw std::runtime_error("叠加层绘制暂不可用");
        const auto scale = static_cast<float>(GetDpiForWindow(window_)) / 96;
        if (!renderer_)
            renderer_ = std::make_unique<render::DirectXRenderer>(window_, embeddedFont(), false, adapter_,
                                                                  render::SurfaceAlpha::Premultiplied);
        if (!visible_)
            ShowWindow(window_, SW_SHOWNOACTIVATE);
        if (!visible_ || width_ != width || height_ != height || scale_ != scale)
            renderer_->submit(scene_, width, height, scale, true);
        visible_ = true;
        bounds_ = r;
        width_ = width;
        height_ = height;
        scale_ = scale;
    }
    catch (const std::exception &e)
    {
        error_ = e.what();
        hide();
    }
}
void OverlaySurface::scene(render::SceneSubmission scene)
{
    scene_ = std::move(scene);
    if (renderer_)
        renderer_->submit(scene_, width_, height_, scale_, visible_);
}
render::RenderStats OverlaySurface::stats() const
{
    auto result = renderer_ ? renderer_->stats() : render::RenderStats{};
    if (!error_.empty())
        result.error = application::storage::wide(error_);
    return result;
}
application::Json OverlaySurface::status() const
{
    const auto s = stats();
    const auto state = !s.error.empty() ? "error" : visible_ ? "ready" : "waiting";
    return {{"placement", "overlay"},
            {"state", state},
            {"visible", visible_},
            {"capture", capture_},
            {"captureApplied", captureApplied_},
            {"renderer", !s.initialized ? "pending"
                         : s.hardware   ? "hardware"
                                        : "software"},
            {"message", !error_.empty()    ? error_
                        : !s.error.empty() ? "叠加层绘制暂不可用"
                        : visible_         ? "已对齐前台游戏窗口"
                                           : "等待本机游戏窗口处于前台"}};
}
application::Json OverlaySurface::identity() const
{
    return {{"overlayHandle", std::to_string(reinterpret_cast<std::uintptr_t>(window_))},
            {"visible", visible_},
            {"x", bounds_.left},
            {"y", bounds_.top},
            {"width", width_},
            {"height", height_},
            {"scale", scale_},
            {"captureApplied", captureApplied_}};
}
} // namespace mousebox::desktop
