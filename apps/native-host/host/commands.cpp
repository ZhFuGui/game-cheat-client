#include "host_state.hpp"
#include "platform/windows/input.hpp"
#include "application/storage/json_store.hpp"
#include "bridge/workspace_scope.hpp"
#include <cmath>
namespace mousebox::desktop
{
void Host::revoke()
{
    eligible = false;
    service->setSessionEligible(false);
}
Json Host::execute(const std::string &command, const Json &payload)
{
    if ((closed || WaitForSingleObject(parent, 0) == WAIT_OBJECT_0) && command != "shutdown" &&
        command != "session.revoke")
        throw std::runtime_error("Native session is closed");
    if (command == "session.hello")
    {
        renewed = mousebox::monotonicMilliseconds();
        eligible = true;
        service->setSessionEligible(true);
        return {{"ok", true}};
    }
    if (command == "session.renew")
    {
        if (!eligible)
            throw std::runtime_error("Session lease revoked; a new handshake is required");
        renewed = mousebox::monotonicMilliseconds();
        return {{"ok", true}};
    }
    if (command == "session.revoke")
    {
        revoke();
        capture->stop();
        surface->hide();
        service->setPresentationActive(false);
        return {{"ok", true}};
    }
    if (command == "shutdown")
    {
        closed = true;
        revoke();
        capture->stop();
        service->shutdown();
        PostQuitMessage(0);
        return {{"ok", true}};
    }
    if (command == "surface.hide")
    {
        capture->stop(true, "bounds");
        surface->hide();
        service->setPresentationActive(false);
        return {{"ok", true}};
    }
    if (command == "surface.configure")
    {
        surface->configure(payload);
        scene();
        return {{"ok", true}};
    }
    if (command == "surface.minimize")
    {
        capture->stop(true, "bounds");
        surface->minimize();
        service->setPresentationActive(surface->visible());
        return {{"ok", true}};
    }
    if (command == "calibration.status")
    {
        const auto s = service->backend().inspectSnapshot();
        return {{"finished", s.calibrationFinished},
                {"succeeded", s.lastMeasurement.measured},
                {"boxReady", s.device.boxReady},
                {"calibrating", s.calibrating}};
    }
    if (command == "render.requestCapture")
    {
        surface->requestCapture();
        return {{"ok", true}};
    }
    if (command == "render.capture")
        return surface->writeCapture(mousebox::application::storage::wide(payload.at("path").get<std::string>()));
    if (command == "render.inspect")
    {
        const auto s = surface->stats();
        return {{"surface", surface->identity()},
                {"initialized", s.initialized},
                {"hardware", s.hardware},
                {"submitted", std::to_string(s.submitted)},
                {"drawn", std::to_string(s.drawn)},
                {"presented", std::to_string(s.presented)},
                {"coalesced", std::to_string(s.coalesced)},
                {"projectionMs", s.projectionMs},
                {"drawSubmitMs", s.drawMs},
                {"deviceRecoveries", std::to_string(s.deviceRecoveries)}};
    }
    if (command == "capture.inspect")
        return capture->inspect();
    if (command == "surface.update")
    {
        if (payload.at("kind") == "esp")
        {
            surface->update(payload);
            service->setPresentationActive(surface->visible());
            return surface->identity();
        }
        else
        {
            if (payload.at("kind") != "learning" && payload.at("kind") != "reports")
                throw std::invalid_argument("Invalid capture surface kind");
            const auto text = payload.at("parentHandle").get<std::string>();
            std::size_t used = 0;
            const auto raw = std::stoull(text, &used);
            if (used != text.size() || text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
                throw std::invalid_argument("Invalid parent handle");
            const auto handle = reinterpret_cast<HWND>(static_cast<std::uintptr_t>(raw));
            DWORD owner = 0;
            GetWindowThreadProcessId(handle, &owner);
            if (owner != parentPid)
                throw std::runtime_error("Invalid learning window");
            const auto scale = payload.value("scale", static_cast<double>(GetDpiForWindow(handle)) / 96.0);
            if (!std::isfinite(scale) || scale <= 0 || scale > 16)
                throw std::invalid_argument("Invalid capture scale");
            const auto x = payload.at("x").get<double>(), y = payload.at("y").get<double>(),
                       w = payload.at("width").get<double>(), h = payload.at("height").get<double>();
            if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(w) || !std::isfinite(h) || x < 0 || y < 0 ||
                w < 0 || h < 0 || x > 32768 || y > 32768 || w > 32768 || h > 32768)
                throw std::invalid_argument("Invalid learning bounds");
            RECT client{};
            GetClientRect(handle, &client);
            if ((x + w) * scale > client.right + 2 || (y + h) * scale > client.bottom + 2)
                throw std::invalid_argument("Learning bounds outside window");
            RECT rect{static_cast<LONG>(x * scale), static_cast<LONG>(y * scale), static_cast<LONG>((x + w) * scale),
                      static_cast<LONG>((y + h) * scale)};
            capture->setBounds(handle, rect);
            if (!payload.at("visible").get<bool>() || payload.value("occluded", false))
                capture->stop(true, "bounds");
        }
        return {{"ok", true}};
    }
    if (command == "surface.palette")
    {
        const auto color = [&](const char *key) {
            return static_cast<std::uint32_t>(std::stoul(payload.at(key).get<std::string>().substr(1), nullptr, 16));
        };
        palette = {color("canvas"), color("grid"), color("guide"), color("text"), color("entity"), color("selected")};
        english = payload.at("language") == "en";
        scene();
        return {{"ok", true}};
    }
    auto result = service->handle(command, payload);
    if (command == "workspace.switch")
    {
        previous = Json::object();
        scene();
    }
    if (command == "settings.set" || command.starts_with("esp."))
    {
        scene();
    }
    if (command == "graphics.refresh")
        surface->refreshAdapter();
    return result;
}
void Host::drain()
{
    std::deque<Json> incoming;
    {
        const std::lock_guard lock(commandsMutex);
        incoming.swap(commands);
    }
    for (const auto &message : incoming)
        pipe->send(requests.dispatch(message, [this, &message](const std::string &command, const Json &payload) {
            if (workspaceScoped(command, payload))
                service->requireWorkspaceContext(message.value("context", Json{}));
            return execute(command, payload);
        }));
}
} // namespace mousebox::desktop
