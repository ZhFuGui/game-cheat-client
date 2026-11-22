#include "options.hpp"
#include "control/hold_plan.hpp"
#include "adapters/outputs/mousebox/serial_sender.hpp"
#include "platform/windows/input.hpp"
#include "platform/windows/ports.hpp"
#include "platform/windows/console.hpp"
#include <Windows.h>
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <iostream>

namespace {
std::atomic<bool> quit{false};
BOOL WINAPI onConsole(DWORD kind) {
    if (kind == CTRL_C_EVENT || kind == CTRL_BREAK_EVENT) { quit = true; return TRUE; }
    return FALSE; // Closing the console/process relies on the device lease as well.
}
}
int controllerEntry(int argc, wchar_t** argv) {
    mousebox::initializeConsole();
    try {
        Options options = parseOptions(argc, argv);
        if (options.help) { printHelp(); return 0; }
        if (options.list) {
            for (const auto& d : mousebox::serialDevices())
                std::wcout << d.port << L"  " << d.name << L"  " << d.identity
                    << (d.ttl ? L"  [USB-TTL]\n" : L"  [非控制模块]\n");
            return 0;
        }
        auto port = mousebox::chooseTtlPort(options.port);
        mousebox::SerialSender sender(port);
        if (options.probe) {
            sender.begin({mousebox_wire::Probe, 0, 0, 0, 0, 0, 0, 0});
            sender.drain(std::chrono::milliseconds(200));
            std::wcout << L"已通过自动波特率与回传核验，无运动探测帧已发送。\n";
            return 0;
        }
        if (!SetConsoleCtrlHandler(onConsole, TRUE)) throw std::runtime_error("Cannot install console cancellation handler");
        const unsigned cost = static_cast<unsigned>(std::abs(options.dx) + std::abs(options.dy));
        const unsigned duration = std::min(1000u, ((options.budget + cost - 1) / cost) * options.interval);
        mousebox::HoldPlan plan(duration); mousebox::PollTimer timer;
        std::uint32_t session = 0, sequence = 0;
        std::wcout << L"已连接 " << port << L"。按住 " << options.keyName << L" 小幅移动，松开停止。\n"
            << L"每次最多 " << options.budget << L" 个计数，最长 " << duration
            << L" ms。Escape / Ctrl+C 退出。启动时需先松开触发键。\n";
        while (!quit && !mousebox::keyDown(VK_ESCAPE)) {
            if (sender.failed()) throw std::runtime_error(sender.error());
            auto action = plan.poll(mousebox::keyDown(options.key), mousebox::monotonicMilliseconds());
            if (action == mousebox::HoldAction::Begin) {
                session = mousebox::randomSession(); sequence = 1;
                sender.begin({mousebox_wire::Start, session, sequence, 0, 0, 0, 0, options.budget});
            }
            if (action == mousebox::HoldAction::Begin || action == mousebox::HoldAction::Refresh) {
                // Recheck after session creation; logging/slow work must not send a stale key-down.
                if (plan.poll(mousebox::keyDown(options.key), mousebox::monotonicMilliseconds()) == mousebox::HoldAction::Stop)
                    action = mousebox::HoldAction::Stop;
                else sender.refresh({mousebox_wire::Motion, session, ++sequence, options.dx, options.dy, options.interval, 80, 0});
            }
            if (action == mousebox::HoldAction::Stop) sender.stop(session);
            timer.wait();
        }
        sender.stop(session);
        sender.drain(std::chrono::milliseconds(200));
        return 0;
    } catch (const std::exception& e) { std::cerr << "ERROR: " << e.what() << "\n"; return 1; }
}
