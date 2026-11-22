#include "host_state.hpp"
#include "platform/windows/input.hpp"
namespace mousebox::desktop
{
void Host::scene()
{
    dirty = false;
    surface->target(service->presentationTarget());
    surface->poll();
    service->setPresentationActive(surface->visible());
    surface->setAdapter(service->values().at("renderAdapter").get<std::string>());
    auto options = service->espOptions();
    if (surface->overlay())
        options.aspect = render::EspAspect::Viewport;
    surface->scene(service->presentation({palette, english, options}));
}
void Host::placement()
{
    if (closed || requests.session().empty())
        return;
    surface->target(service->presentationTarget());
    if (surface->poll())
        scene();
}
void Host::events()
{
    if (requests.session().empty() || closed)
        return;
    if (captureOverflow.exchange(false))
        capture->stop(false);
    service->poll();
    service->setRenderStats(surface->stats());
    auto next = service->snapshot();
    next["state"]["espPresentation"] = surface->status();
    const auto context = service->workspaceContext();
    if (next.contains("workspace") && (!previous.contains("workspace") || previous["workspace"] != next["workspace"]))
    {
        // Catalog is always delivered first, even while the renderer is minimized.
        pipe->send(
            {{"version", 1}, {"session", requests.session()}, {"topic", "workspace"}, {"data", next["workspace"]}});
        previous = Json::object();
    }
    if (next.contains("application") &&
        (!previous.contains("application") || previous["application"] != next["application"]))
        pipe->send(
            {{"version", 1}, {"session", requests.session()}, {"topic", "application"}, {"data", next["application"]}});
    if (next.contains("devices") && (!previous.contains("devices") || previous["devices"] != next["devices"]))
        pipe->send({{"version", 1}, {"session", requests.session()}, {"topic", "devices"}, {"data", next["devices"]}});
    for (const auto name :
         {"values", "espValues", "speedPresets", "state", "learning", "mouseReports", "ports", "graphicsAdapters"})
        if (next.contains(name) && (!previous.contains(name) || previous[name] != next[name]))
            pipe->send({{"version", 1},
                        {"session", requests.session()},
                        {"topic", name},
                        {"data", next[name]},
                        {"context", context}});
    if (next.contains("algorithm") && (!previous.contains("algorithm") || previous["algorithm"] != next["algorithm"]))
        pipe->send({{"version", 1},
                    {"session", requests.session()},
                    {"topic", "algorithm"},
                    {"data", {{"algorithm", next["algorithm"]}, {"algorithmSections", next["algorithmSections"]}}},
                    {"context", context}});
    const auto logs = service->handle("logs.read", {{"after", logCursor}, {"limit", 500}});
    logCursor = logs.at("cursor").get<std::string>();
    if (!logs.at("entries").empty())
        pipe->send({{"version", 1}, {"session", requests.session()}, {"topic", "logs"}, {"data", logs}});
    previous = next;
}
} // namespace mousebox::desktop
