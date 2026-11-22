#include "frame_clock.hpp"
#include <Windows.h>
namespace mousebox::render
{
std::optional<double> frameClockMilliseconds() noexcept
{
    static const auto frequency = []() noexcept {
        LARGE_INTEGER value{};
        if (!QueryPerformanceFrequency(&value) || value.QuadPart <= 0)
            return 0.L;
        return static_cast<long double>(value.QuadPart);
    }();
    LARGE_INTEGER tick{};
    if (!frequency || !QueryPerformanceCounter(&tick) || tick.QuadPart < 0)
        return {};
    return static_cast<double>(static_cast<long double>(tick.QuadPart) * 1000 / frequency);
}
} // namespace mousebox::render
