#include "adapters/sources/pubg_reader/runtime.hpp"
#include "platform/windows/console.hpp"
#include <Windows.h>
#include <winsock2.h>
#include <bcrypt.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
std::atomic<bool> quit = false;
std::atomic<bool> fatal = false;
BOOL WINAPI control(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT || signal == CTRL_CLOSE_EVENT) { quit = true; return TRUE; }
    return FALSE;
}
unsigned number(const std::string& value, unsigned maximum) {
    if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos) throw std::runtime_error("Invalid numeric option");
    std::size_t used = 0; const auto parsed = std::stoul(value,&used);
    if (used != value.size() || parsed > maximum) throw std::runtime_error("Invalid numeric option");
    return static_cast<unsigned>(parsed);
}
}
int readerEntry(int argc, char** argv) {
    mousebox::initializeConsole();
    try {
        mousebox::reader::Options options;
        unsigned long long instance = 0;
        if (BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&instance),sizeof(instance),BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0 || !instance)
            throw std::runtime_error("Client instance random generation failed");
        options.instance = "mousebox-"+std::to_string(instance);
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--help") {
                std::cout << "Reader 只读观察：--host IPv4 --port 13102 --hz 30 --seconds 10 --no-probe --no-clock\n"
                             "--reconnect-living-after 秒：仅重连 Living 一次；Living/Local 独立连接；Ctrl+C 退出。\n"; return 0;
            }
            if (arg == "--no-probe") { options.probes = false; continue; }
            if (arg == "--no-clock") { options.declareClock = false; options.probes = false; continue; }
            if (++i >= argc) throw std::runtime_error("Option value missing");
            const std::string value = argv[i];
            if (arg == "--host") options.host = value;
            else if (arg == "--evidence-dir") options.evidenceDirectory = value;
            else if (arg == "--port") { const auto port = number(value,65535); if (!port) throw std::runtime_error("Zero port"); options.port = static_cast<unsigned short>(port); }
            else if (arg == "--seconds") options.seconds = number(value,86400);
            else if (arg == "--reconnect-living-after") options.reconnectLivingAfterSeconds = number(value,86400);
            else if (arg == "--hz") { std::size_t used = 0; options.hz = std::stod(value,&used);
                if (used != value.size() || !std::isfinite(options.hz) || options.hz <= 0 || options.hz > 144) throw std::runtime_error("Invalid Hz"); }
            else throw std::runtime_error("Unknown option: "+arg);
        }
        WSADATA data{}; if (WSAStartup(MAKEWORD(2,2),&data)) throw std::runtime_error("Winsock startup failed");
        SetConsoleCtrlHandler(control,TRUE);
        {
            const auto worker = [&](unsigned category, std::stop_token stop) {
                try { mousebox::reader::observe(options,category,stop); }
                catch (const std::exception& e) { mousebox::reader::report(e.what()); fatal = true; quit = true; }
            };
            std::jthread living([&](std::stop_token stop) { worker(1,stop); });
            std::jthread view([&](std::stop_token stop) { worker(6,stop); });
            const auto start = std::chrono::steady_clock::now();
            while (!quit && (!options.seconds || std::chrono::steady_clock::now()-start < std::chrono::seconds(options.seconds)))
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            living.request_stop(); view.request_stop();
        }
        WSACleanup(); return fatal ? 1 : 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
