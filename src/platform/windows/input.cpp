#include "platform/windows/input.hpp"
#include <Windows.h>
#include <bcrypt.h>
#include <stdexcept>

namespace mousebox {
bool keyDown(int virtualKey) { return (GetAsyncKeyState(virtualKey) & 0x8000) != 0; }
std::uint32_t randomSession() {
    std::uint32_t session = 0;
    while (!session) if (BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(&session), sizeof(session),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) throw std::runtime_error("Cannot generate control session ID");
    return session;
}
std::uint64_t monotonicMilliseconds() {
    static const auto frequency=[] {
        LARGE_INTEGER value{};
        if(!QueryPerformanceFrequency(&value)||value.QuadPart<=0) throw std::runtime_error("Cannot query monotonic clock frequency");
        return static_cast<std::uint64_t>(value.QuadPart);
    }();
    LARGE_INTEGER value{};
    if(!QueryPerformanceCounter(&value)||value.QuadPart<0) throw std::runtime_error("Cannot query monotonic clock");
    const auto ticks=static_cast<std::uint64_t>(value.QuadPart);
    return ticks/frequency*1000+static_cast<std::uint64_t>(static_cast<long double>(ticks%frequency)*1000/frequency);
}
PollTimer::PollTimer() {
    handle_ = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (!handle_) handle_ = CreateWaitableTimerW(nullptr, FALSE, nullptr);
    if (!handle_) throw std::runtime_error("Cannot create keyboard poll timer");
}
PollTimer::~PollTimer() { if (handle_) CloseHandle(handle_); }
void PollTimer::wait(unsigned milliseconds) {
    if(milliseconds<1||milliseconds>20) throw std::invalid_argument("Invalid poll interval");
    LARGE_INTEGER due{}; due.QuadPart = -static_cast<LONGLONG>(milliseconds)*10000; // No global timer-resolution change.
    if (!SetWaitableTimer(handle_, &due, 0, nullptr, nullptr, FALSE) ||
        WaitForSingleObject(handle_, 100) != WAIT_OBJECT_0) throw std::runtime_error("Keyboard timer failed");
}
}
