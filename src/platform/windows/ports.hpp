#pragma once
#include <string>
#include <vector>

namespace mousebox {
struct SerialDevice { std::wstring port, name, identity; bool ttl = false; };
std::vector<SerialDevice> serialDevices();
std::wstring chooseTtlPort(const std::wstring& requested);
}
