#pragma once
#include <Windows.h>
#include <string>
#include <span>
#include <cstdint>

namespace mousebox
{
class SerialPort
{
    HANDLE port_ = INVALID_HANDLE_VALUE, event_ = nullptr, readEvent_ = nullptr;

  public:
    SerialPort(const std::wstring &port, unsigned baudRate);
    ~SerialPort();
    SerialPort(const SerialPort &) = delete;
    SerialPort &operator=(const SerialPort &) = delete;
    // Call only while no read/write operation is in flight.
    void setBaudRate(unsigned baudRate);
    void write(std::span<const std::uint8_t> frame);
    std::size_t readAvailable(std::span<std::uint8_t>);
};
} // namespace mousebox
