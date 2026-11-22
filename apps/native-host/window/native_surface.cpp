#include "native_surface.hpp"
#include "resources/embedded_font.hpp"
#include <cmath>
#include <stdexcept>
#include <fstream>
namespace mousebox::desktop
{
NativeSurface::~NativeSurface()
{
    renderer_.reset();
    if (child_)
        DestroyWindow(child_);
}
void NativeSurface::update(const application::Json &p)
{
    const auto text = p.at("parentHandle").get<std::string>();
    std::size_t used = 0;
    const auto raw = std::stoull(text, &used);
    if (used != text.size() || text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("Invalid parent handle");
    const auto parent = reinterpret_cast<HWND>(static_cast<std::uintptr_t>(raw));
    DWORD owner = 0;
    GetWindowThreadProcessId(parent, &owner);
    if (!IsWindow(parent) || owner != parentPid_ || GetAncestor(parent, GA_ROOT) != parent)
        throw std::invalid_argument("Invalid parent window");
    if (parent_ && parent_ != parent)
        throw std::invalid_argument("Parent window changed without a new native session");
    const auto x = p.at("x").get<double>(), y = p.at("y").get<double>(), w = p.at("width").get<double>(),
               h = p.at("height").get<double>();
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(w) || !std::isfinite(h) || x < 0 || y < 0 || w < 0 ||
        h < 0 || x > 32768 || y > 32768 || w > 32768 || h > 32768)
        throw std::invalid_argument("Invalid native bounds");
    const auto scale = p.value("scale", static_cast<double>(GetDpiForWindow(parent)) / 96.0);
    if (!std::isfinite(scale) || scale <= 0 || scale > 16)
        throw std::invalid_argument("Invalid surface scale");
    parent_ = parent;
    scale_ = static_cast<float>(scale);
    RECT client{};
    GetClientRect(parent, &client);
    const int px = static_cast<int>(std::lround(x * scale_)), py = static_cast<int>(std::lround(y * scale_));
    const int width = static_cast<int>(std::lround(w * scale_)), height = static_cast<int>(std::lround(h * scale_));
    if (px + width > client.right + 2 || py + height > client.bottom + 2)
        throw std::invalid_argument("Native bounds outside client area");
    visible_ = p.at("visible").get<bool>() && !p.value("occluded", false) && !IsIconic(parent) &&
               IsWindowVisible(parent) && width > 0 && height > 0;
    if (!child_ && p.value("attach", p.at("visible").get<bool>()) && width > 0 && height > 0)
    {
        child_ =
            CreateWindowExW(WS_EX_NOACTIVATE, L"STATIC", L"Mousebox GPU", WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                            px, py, width, height, parent, nullptr, GetModuleHandleW(nullptr), nullptr);
        if (!child_)
            throw std::runtime_error("Could not create native GPU surface");
        renderer_ = std::make_unique<render::DirectXRenderer>(child_, embeddedFont(), false, requestedAdapter_);
    }
    width_ = static_cast<unsigned>(width);
    height_ = static_cast<unsigned>(height);
    x_ = px;
    y_ = py;
    if (child_)
    {
        SetWindowPos(child_, HWND_TOP, px, py, width, height, SWP_NOACTIVATE);
        ShowWindow(child_, visible_ ? SW_SHOWNOACTIVATE : SW_HIDE);
    }
    if (renderer_)
        renderer_->submit(scene_, width_, height_, scale_, visible_);
}
void NativeSurface::hide()
{
    visible_ = false;
    if (child_)
        ShowWindow(child_, SW_HIDE);
    if (renderer_)
        renderer_->submit(scene_, width_, height_, scale_, false);
}
void NativeSurface::scene(render::SceneSubmission scene)
{
    scene_ = std::move(scene);
    if (renderer_)
        renderer_->submit(scene_, width_, height_, scale_, visible_);
}
void NativeSurface::setAdapter(std::string id)
{
    requestedAdapter_ = std::move(id);
    if (renderer_)
        renderer_->setAdapter(requestedAdapter_);
}
void NativeSurface::refreshAdapter()
{
    if (renderer_)
        renderer_->recreateDevice();
}
render::RenderStats NativeSurface::stats() const
{
    return renderer_ ? renderer_->stats() : render::RenderStats{};
}
application::Json NativeSurface::identity() const
{
    RECT client{};
    if (parent_)
        GetClientRect(parent_, &client);
    return {{"ok", true},
            {"childHandle", std::to_string(reinterpret_cast<std::uintptr_t>(child_))},
            {"parentHandle", std::to_string(reinterpret_cast<std::uintptr_t>(child_ ? GetParent(child_) : parent_))},
            {"dpi", parent_ ? GetDpiForWindow(parent_) : 96u},
            {"clientWidth", client.right},
            {"clientHeight", client.bottom},
            {"scale", scale_},
            {"visible", visible_}};
}
void NativeSurface::requestCapture()
{
    if (renderer_)
        renderer_->requestCapture();
}
application::Json NativeSurface::writeCapture(const std::filesystem::path &path) const
{
    if (!visible_)
        return {{"available", false}};
    const auto pixels = renderer_ ? renderer_->capture() : render::Capture{};
    if (pixels.bgra.empty())
        return {{"available", false}};
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char *>(pixels.bgra.data()), static_cast<std::streamsize>(pixels.bgra.size()));
    if (!file)
        throw std::runtime_error("Could not save diagnostic GPU capture");
    return {{"available", true}, {"width", pixels.width}, {"height", pixels.height}, {"x", x_},
            {"y", y_},           {"scale", scale_}};
}
} // namespace mousebox::desktop
