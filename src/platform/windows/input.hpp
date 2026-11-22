#pragma once
#include <cstdint>

namespace mousebox {
bool keyDown(int virtualKey);
std::uint32_t randomSession();
// Consumer-local QPC milliseconds. All local freshness and control timestamps use this domain.
std::uint64_t monotonicMilliseconds();
class PollTimer {
    void* handle_ = nullptr;
public:
    PollTimer();
    ~PollTimer();
    PollTimer(const PollTimer&) = delete;
    PollTimer& operator=(const PollTimer&) = delete;
    void wait(unsigned milliseconds=2);
};
}
