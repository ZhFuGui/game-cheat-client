#include "platform/windows/qpc.hpp"
#include <Windows.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace mousebox
{
std::uint64_t readQpcTicks()
{
    LARGE_INTEGER ticks{};
    if (!QueryPerformanceCounter(&ticks) || ticks.QuadPart < 0)
        throw std::runtime_error("QPC sample failed");
    return static_cast<std::uint64_t>(ticks.QuadPart);
}
QpcStamp readQpc()
{
    using NtQuery = LONG(NTAPI *)(ULONG, PVOID, ULONG, PULONG);
    struct BootEnvironment
    {
        GUID identifier;
        ULONG firmwareType;
        ULONGLONG flags;
    };
    auto query = reinterpret_cast<NtQuery>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQuerySystemInformation"));
    BootEnvironment boot{};
    ULONG bytes = 0;
    LARGE_INTEGER ticks{}, frequency{};
    const GUID zero{};
    if (!query || query(90, &boot, sizeof(boot), &bytes) < 0 ||
        std::memcmp(&boot.identifier, &zero, sizeof(zero)) == 0 || !QueryPerformanceFrequency(&frequency) ||
        !QueryPerformanceCounter(&ticks) || frequency.QuadPart <= 0 || ticks.QuadPart <= 0)
        throw std::runtime_error("Native boot/QPC query failed");
    const auto &g = boot.identifier;
    std::array<char, 80> domain{};
    std::snprintf(domain.data(), domain.size(), "windows-qpc:%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                  static_cast<unsigned long>(g.Data1), g.Data2, g.Data3, g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
                  g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
    return {domain.data(), static_cast<std::uint64_t>(ticks.QuadPart), static_cast<std::uint64_t>(frequency.QuadPart)};
}
} // namespace mousebox
