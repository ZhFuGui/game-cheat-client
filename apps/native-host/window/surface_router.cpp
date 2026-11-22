#include "surface_router.hpp"
#include <stdexcept>
namespace mousebox::desktop
{
void SurfaceRouter::applyAdapter()
{
    const auto value = renderer_ == "software" && !target_.executable.empty() ? "warp" : adapter_;
    embedded_.setAdapter(value);
    overlay_.adapter(value);
}
void SurfaceRouter::configure(const application::Json &p)
{
    const auto placement = p.at("placement").get<std::string>(), capture = p.at("capture").get<std::string>(),
               renderer = p.at("renderer").get<std::string>();
    if (p.size() != 3 || (placement != "embedded" && placement != "overlay") ||
        (capture != "include" && capture != "exclude") || (renderer != "default" && renderer != "software"))
        throw std::invalid_argument("Invalid ESP presentation options");
    const bool modeChanged = placement_ != placement;
    if (modeChanged)
    {
        embedded_.hide();
        overlay_.hide();
    }
    placement_ = placement;
    capture_ = capture;
    renderer_ = renderer;
    // Reconfiguration is resumed only by the coordinator's next bounds request,
    // which carries the current page/dialog visibility rather than stale flags.
    requested_ = false;
    applyAdapter();
    if (overlay())
        overlay_.configure(capture_);
    submitScene();
}
void SurfaceRouter::update(const application::Json &p)
{
    const auto text = p.at("parentHandle").get<std::string>();
    std::size_t used = 0;
    const auto raw = std::stoull(text, &used);
    if (text.empty() || used != text.size() || text.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("Invalid parent handle");
    const auto parent = reinterpret_cast<HWND>(static_cast<std::uintptr_t>(raw));
    DWORD owner = 0;
    GetWindowThreadProcessId(parent, &owner);
    if (!IsWindow(parent) || owner != parentPid_ || GetAncestor(parent, GA_ROOT) != parent ||
        (parent_ && parent_ != parent))
        throw std::invalid_argument("Invalid ESP parent window");
    parent_ = parent;
    requested_ = p.value("active", p.at("visible").get<bool>()) && !p.value("occluded", false);
    if (overlay())
    {
        embedded_.hide();
        overlay_.poll(target_, requested_);
    }
    else
    {
        auto child = p;
        child["visible"] = p.at("visible").get<bool>() && requested_;
        embedded_.update(child);
    }
}
void SurfaceRouter::target(application::ports::GameWindowTarget next)
{
    if (target_.executable != next.executable || target_.process != next.process)
    {
        embedded_.hide();
        overlay_.hide();
        overlay_.scene({});
        target_ = std::move(next);
        applyAdapter();
        if (overlay())
            overlay_.configure(capture_);
    }
}
bool SurfaceRouter::poll()
{
    const bool before = visible();
    if (overlay())
        overlay_.poll(target_, requested_);
    return before != visible();
}
void SurfaceRouter::hide()
{
    requested_ = false;
    embedded_.hide();
    overlay_.hide();
}
void SurfaceRouter::minimize()
{
    embedded_.hide();
    if (!overlay())
        requested_ = false;
}
void SurfaceRouter::scene(application::ports::PresentationFrame frame)
{
    scene_ = withDrawCompletion(std::move(frame), completed_);
    submitScene();
}
void SurfaceRouter::submitScene()
{
    if (overlay())
        overlay_.scene(scene_);
    else
        embedded_.scene(scene_);
}
void SurfaceRouter::setAdapter(std::string id)
{
    if (adapter_ != id)
    {
        adapter_ = std::move(id);
        applyAdapter();
    }
}
void SurfaceRouter::refreshAdapter()
{
    embedded_.refreshAdapter();
    overlay_.refreshAdapter();
}
render::RenderStats SurfaceRouter::stats() const
{
    return overlay() ? overlay_.stats() : embedded_.stats();
}
application::Json SurfaceRouter::identity() const
{
    auto result = embedded_.identity();
    result["placement"] = overlay() ? "overlay" : "embedded";
    result["requestedPlacement"] = placement_;
    result["requested"] = requested_;
    if (overlay())
        result["overlay"] = overlay_.identity();
    return result;
}
application::Json SurfaceRouter::status() const
{
    if (overlay())
        return overlay_.status();
    const auto s = embedded_.stats();
    return {{"placement", "embedded"},
            {"state", embedded_.visible() ? "ready" : "idle"},
            {"visible", embedded_.visible()},
            {"capture", "include"},
            {"captureApplied", false},
            {"renderer", !s.initialized ? "pending" : s.hardware ? "hardware" : "software"},
            {"message", "内嵌 ESP 画布"}};
}
void SurfaceRouter::requestCapture()
{
    if (!overlay())
        embedded_.requestCapture();
}
application::Json SurfaceRouter::writeCapture(const std::filesystem::path &path) const
{
    return overlay() ? application::Json{{"available", false}} : embedded_.writeCapture(path);
}
} // namespace mousebox::desktop
