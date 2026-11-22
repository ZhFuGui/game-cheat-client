#pragma once
namespace mousebox::control
{
enum class OutputMode
{
    Idle,
    DesktopTest,
    AutomaticAim
};
// There is no persistent aim-enable latch. Explicit test/capture/calibration
// owns the output temporarily; ordinary settings and page changes do not.
constexpr OutputMode outputMode(bool readerRequested, bool boxReady, bool test, bool calibrating,
                                bool capturing) noexcept
{
    if (!boxReady || calibrating || capturing)
        return OutputMode::Idle;
    if (test)
        return OutputMode::DesktopTest;
    return readerRequested ? OutputMode::AutomaticAim : OutputMode::Idle;
}
} // namespace mousebox::control
