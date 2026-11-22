#include <winsock2.h>
#include "desktop_host.hpp"
#include "host_state.hpp"
#include "platform/windows/input.hpp"
#include "application/storage/json_store.hpp"
#include "composition/game_modules.hpp"
#include <array>
#include <filesystem>
#include <fstream>
namespace mousebox::desktop
{
LRESULT CALLBACK procedure(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    auto host = reinterpret_cast<Host *>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        host = static_cast<Host *>(reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(host));
    }
    if (host)
    {
        try
        {
            if (message == commandMessage)
            {
                host->drain();
                return 0;
            }
            if (message == sceneMessage)
            {
                host->scene();
                return 0;
            }
            if (message == WM_TIMER)
            {
                if (wp == 1)
                    host->events();
                else if (wp == 2)
                    host->placement();
                return 0;
            }
            if (message == WM_CLOSE)
            {
                host->revoke();
                if (host->capture)
                    host->capture->stop();
                PostQuitMessage(0);
                return 0;
            }
        }
        catch (const std::exception &error)
        {
            host->revoke();
            try
            {
                if (host->capture)
                    host->capture->stop();
                if (host->pipe && !host->requests.session().empty())
                    host->pipe->send({{"version", 1},
                                      {"session", host->requests.session()},
                                      {"topic", "error"},
                                      {"data", {{"code", "native_runtime"}, {"message", error.what()}}}});
            }
            catch (const std::exception &)
            {
            }
            return 0;
        }
    }
    return DefWindowProcW(window, message, wp, lp);
}
int runDesktopHost(int argc, wchar_t **argv)
{
    struct Winsock
    {
        Winsock()
        {
            WSADATA data{};
            if (WSAStartup(MAKEWORD(2, 2), &data))
                throw std::runtime_error("Winsock startup failed");
        }
        ~Winsock()
        {
            WSACleanup();
        }
    } winsock;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    Host host;
    std::filesystem::path data, imported;
    std::string captureToken;
    for (int i = 2; i < argc; ++i)
    {
        const std::wstring key = argv[i];
        if (i + 1 >= argc)
            throw std::invalid_argument("Missing bridge argument");
        if (key == L"--data-dir")
            data = argv[++i];
        else if (key == L"--parent-pid")
            host.parentPid = std::stoul(argv[++i]);
        else if (key == L"--import-settings")
            imported = argv[++i];
        else if (key == L"--capture-token")
            captureToken = mousebox::application::storage::utf8(argv[++i]);
        else
            throw std::invalid_argument("Unknown bridge argument");
    }
    if (data.empty() || !host.parentPid)
        throw std::invalid_argument("Bridge requires a data directory and parent pid");
    host.parent = OpenProcess(SYNCHRONIZE, FALSE, host.parentPid);
    if (!host.parent)
        throw std::runtime_error("Parent process is not available");
    std::optional<Json> initial;
    if (!imported.empty())
    {
        std::ifstream file(imported);
        if (!file)
            throw std::runtime_error("Could not open imported settings");
        initial = Json::parse(file);
    }
    std::array<wchar_t, 32768> executable{};
    GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
    host.service = std::make_unique<mousebox::application::RuntimeService>(mousebox::application::RuntimeOptions{
        data, std::filesystem::path(executable.data()).parent_path(), initial, composition::createGameModule});
    const WNDCLASSW cls{0,       procedure, 0,       0,       GetModuleHandleW(nullptr),
                        nullptr, nullptr,   nullptr, nullptr, L"MouseboxNativeMessages"};
    RegisterClassW(&cls);
    host.window =
        CreateWindowExW(0, cls.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, cls.hInstance, &host);
    if (!host.window)
        throw std::runtime_error("Native message window failed");
    host.surface = std::make_unique<mousebox::desktop::SurfaceRouter>(
        host.parentPid, [&](std::span<const mousebox::domain::SourceSample> headers, double completedAtMs) {
            host.service->drawCompleted(headers, completedAtMs);
        });
    host.capture = std::make_unique<mousebox::desktop::CaptureClient>(host.parentPid, std::move(captureToken));
    host.capture->onStatus = [&](bool active, std::string text, double x, double y) {
        host.service->setCaptureStatus(active, std::move(text), x, y);
    };
    host.capture->onTrials = [&](auto trials) {
        if (!host.service->appendCapturedTrials(std::move(trials)))
        {
            host.captureOverflow = true;
            host.service->setCaptureStatus(false, "采样已满，请导出并清空");
        }
    };
    host.service->setCaptureHooks([&](unsigned dpi) { host.capture->start(dpi); }, [&] { host.capture->stop(); },
                                  [&] { host.capture->clear(); });
    host.capture->onReportStatus = [&](auto progress) { host.service->setReportCaptureStatus(std::move(progress)); };
    host.capture->onReports = [&](auto recording) { host.service->acceptMouseReports(std::move(recording)); };
    host.service->setReportCaptureHook([&] { host.capture->startReports(); });
    host.capture->connect();
    host.pipe = std::make_unique<mousebox::desktop::Transport>(
        [&](Json value) {
            if (!value.is_object())
                throw std::invalid_argument("Envelope must be an object");
            {
                const std::lock_guard lock(host.commandsMutex);
                if (host.commands.size() >= 128)
                    throw std::runtime_error("Command queue full");
                host.commands.push_back(std::move(value));
            }
            PostMessageW(host.window, commandMessage, 0, 0);
        },
        [&] {
            host.closed = true;
            host.revoke();
            try
            {
                host.capture->stop();
            }
            catch (const std::exception &)
            {
            }
            PostMessageW(host.window, WM_CLOSE, 0, 0);
        });
    host.service->setSceneNotifier([&] {
        if (!host.dirty.exchange(true))
            PostMessageW(host.window, sceneMessage, 0, 0);
    });
    host.watchdog = std::jthread([&](std::stop_token stop) {
        while (!stop.stop_requested())
        {
            if (WaitForSingleObject(host.parent, 50) == WAIT_OBJECT_0 || host.closed)
            {
                host.closed = true;
                host.revoke();
                try
                {
                    host.capture->stop();
                }
                catch (const std::exception &)
                {
                }
                PostMessageW(host.window, WM_CLOSE, 0, 0);
                return;
            }
            if (host.eligible && mousebox::monotonicMilliseconds() - host.renewed > 3000)
            {
                host.revoke();
                try
                {
                    host.capture->stop();
                }
                catch (const std::exception &)
                {
                }
                PostMessageW(host.window, WM_APP + 3, 0, 0);
            }
        }
    });
    SetTimer(host.window, 1, 100, nullptr);
    // Window tracking only; unchanged geometry does not submit or limit GPU frames.
    SetTimer(host.window, 2, 16, nullptr);
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        if (message.message == WM_APP + 3)
        {
            host.capture->stop();
            host.surface->hide();
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    host.revoke();
    host.capture->stop();
    host.service->setSceneNotifier({});
    host.service->shutdown();
    host.watchdog.request_stop();
    host.watchdog.join();
    host.pipe->flush();
    host.pipe.reset();
    host.capture.reset();
    host.surface.reset();
    DestroyWindow(host.window);
    CloseHandle(host.parent);
    return 0;
}
} // namespace mousebox::desktop
