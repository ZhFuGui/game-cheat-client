#include "foreground_process.hpp"
#include <string>
#include <Windows.h>
namespace mousebox
{
std::uint32_t foregroundProcessId() noexcept
{
    DWORD process = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &process);
    return process;
}
bool foregroundExecutable(std::string_view filename) noexcept
{
    const auto pid = foregroundProcessId();
    if (!pid || filename.empty() || filename.size() > 240)
        return false;
    const auto process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process)
        return false;
    wchar_t path[32768];
    DWORD count = static_cast<DWORD>(std::size(path));
    const bool queried = QueryFullProcessImageNameW(process, 0, path, &count) != FALSE;
    CloseHandle(process);
    if (!queried)
        return false;
    wchar_t expected[241];
    const auto length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename.data(),
                                            static_cast<int>(filename.size()), expected, 240);
    if (length <= 0)
        return false;
    const std::wstring_view full(path, count);
    const auto separator = full.find_last_of(L"\\/");
    const auto basename = separator == full.npos ? full : full.substr(separator + 1);
    return CompareStringOrdinal(basename.data(), static_cast<int>(basename.size()), expected, length, TRUE) ==
           CSTR_EQUAL;
}
} // namespace mousebox
