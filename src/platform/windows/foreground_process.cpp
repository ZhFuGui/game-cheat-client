#include "foreground_process.hpp"
#include <Windows.h>
namespace mousebox
{
std::uint32_t foregroundProcessId() noexcept
{
    DWORD process = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &process);
    return process;
}
} // namespace mousebox
