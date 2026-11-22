#include "platform/windows/ports.hpp"
#include <Windows.h>
#include <SetupAPI.h>
#include <devguid.h>
#include <algorithm>
#include <cwctype>
#include <stdexcept>

namespace mousebox {
namespace {
bool knownTtl(const std::wstring& identity) {
    const wchar_t* ids[] = {L"VID_10C4&PID_EA60", L"VID_1A86&PID_7523", L"VID_1A86&PID_5523",
                           L"VID_0403+PID_6001", L"VID_0403&PID_6001", L"VID_067B&PID_2303"};
    for (auto id : ids) if (identity.find(id) != std::wstring::npos) return true;
    return false;
}
std::wstring upper(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t c) { return std::towupper(c); });
    return value;
}
}
std::vector<SerialDevice> serialDevices() {
    HDEVINFO devices = SetupDiGetClassDevsW(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);
    if (devices == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot enumerate serial devices");
    std::vector<SerialDevice> result;
    SP_DEVINFO_DATA info{}; info.cbSize = sizeof(info);
    for (DWORD index = 0; SetupDiEnumDeviceInfo(devices, index, &info); ++index) {
        wchar_t name[512]{}, identity[512]{}, port[64]{};
        SetupDiGetDeviceRegistryPropertyW(devices, &info, SPDRP_FRIENDLYNAME, nullptr,
            reinterpret_cast<PBYTE>(name), sizeof(name), nullptr);
        SetupDiGetDeviceInstanceIdW(devices, &info, identity, 512, nullptr);
        HKEY key = SetupDiOpenDevRegKey(devices, &info, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        DWORD size = sizeof(port), type = 0;
        if (key != INVALID_HANDLE_VALUE) {
            RegQueryValueExW(key, L"PortName", nullptr, &type, reinterpret_cast<BYTE*>(port), &size);
            RegCloseKey(key);
        }
        if (type == REG_SZ && port[0]) result.push_back({port, name, identity, knownTtl(upper(identity))});
    }
    SetupDiDestroyDeviceInfoList(devices);
    return result;
}
std::wstring chooseTtlPort(const std::wstring& requested) {
    auto devices = serialDevices();
    std::vector<SerialDevice> matching;
    for (const auto& device : devices)
        if ((requested.empty() && device.ttl) || (!requested.empty() && upper(device.port) == upper(requested)))
            matching.push_back(device);
    if (matching.size() != 1) throw std::runtime_error("Select one connected USB-TTL port with --port COMn; use --list");
    if (!matching.front().ttl) throw std::runtime_error("Port is not a supported USB-TTL adapter; native USB CDC is diagnostic only");
    return matching.front().port;
}
}
