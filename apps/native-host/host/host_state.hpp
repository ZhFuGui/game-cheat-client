#pragma once
#include "bridge/transport.hpp"
#include "bridge/runtime_service.hpp"
#include "bridge/request_ledger.hpp"
#include "window/surface_router.hpp"
#include "input/capture_client.hpp"
#include <atomic>
#include <deque>
#include <mutex>
#include <thread>
namespace mousebox::desktop
{
using application::Json;
inline constexpr UINT commandMessage = WM_APP + 1, sceneMessage = WM_APP + 2;
// Composition root state: resources live and are destroyed on the host message thread.
struct Host
{
    HWND window = nullptr;
    DWORD parentPid = 0;
    HANDLE parent = nullptr;
    std::unique_ptr<mousebox::application::RuntimeService> service;
    std::unique_ptr<mousebox::desktop::Transport> pipe;
    std::unique_ptr<mousebox::desktop::SurfaceRouter> surface;
    std::unique_ptr<mousebox::desktop::CaptureClient> capture;
    std::atomic<bool> captureOverflow = false;
    std::mutex commandsMutex;
    std::deque<Json> commands;
    std::atomic<bool> dirty = false, closed = false, eligible = false;
    std::atomic<std::uint64_t> renewed = 0;
    std::jthread watchdog;
    Json previous = Json::object();
    RequestLedger requests;
    std::string logCursor;
    mousebox::render::Palette palette;
    bool english = false;
    void revoke();
    void scene();
    void placement();
    void events();
    Json execute(const std::string &, const Json &);
    void drain();
};
} // namespace mousebox::desktop
