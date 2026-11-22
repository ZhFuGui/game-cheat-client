#include "platform/windows/serial_port.hpp"
#include <stdexcept>

namespace mousebox
{
namespace
{
[[noreturn]] void fail(const char *stage)
{
    throw std::runtime_error(std::string(stage) + ": Windows error " + std::to_string(GetLastError()));
}
} // namespace
SerialPort::SerialPort(const std::wstring &port, unsigned baudRate)
{
    event_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!event_)
        fail("CreateEvent");
    readEvent_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!readEvent_)
    {
        CloseHandle(event_);
        event_ = nullptr;
        fail("Create read event");
    }
    port_ = CreateFileW((L"\\\\.\\" + port).c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                        FILE_FLAG_OVERLAPPED, nullptr);
    if (port_ == INVALID_HANDLE_VALUE)
    {
        DWORD code = GetLastError();
        CloseHandle(event_);
        CloseHandle(readEvent_);
        event_ = readEvent_ = nullptr;
        throw std::runtime_error("Open serial port: Windows error " + std::to_string(code));
    }
    try
    {
        setBaudRate(baudRate);
        COMMTIMEOUTS timeouts{};
        timeouts.WriteTotalTimeoutConstant = 20;
        timeouts.ReadIntervalTimeout = MAXDWORD;
        if (!SetCommTimeouts(port_, &timeouts))
            fail("SetCommTimeouts");
        if (!PurgeComm(port_, PURGE_TXABORT | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_RXCLEAR))
            fail("PurgeComm");
    }
    catch (...)
    {
        CloseHandle(port_);
        CloseHandle(event_);
        CloseHandle(readEvent_);
        port_ = INVALID_HANDLE_VALUE;
        event_ = readEvent_ = nullptr;
        throw;
    }
}
SerialPort::~SerialPort()
{
    if (port_ != INVALID_HANDLE_VALUE)
        CloseHandle(port_);
    if (event_)
        CloseHandle(event_);
    if (readEvent_)
        CloseHandle(readEvent_);
}
void SerialPort::setBaudRate(unsigned baudRate)
{
    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(port_, &dcb))
        fail("GetCommState");
    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = dcb.fOutxDsrFlow = dcb.fDsrSensitivity = FALSE;
    dcb.fOutX = dcb.fInX = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fAbortOnError = FALSE;
    if (!SetCommState(port_, &dcb))
        fail("SetCommState");
    if (!PurgeComm(port_, PURGE_TXABORT | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_RXCLEAR))
        fail("PurgeComm");
}
std::size_t SerialPort::readAvailable(std::span<std::uint8_t> bytes)
{
    OVERLAPPED operation{};
    operation.hEvent = readEvent_;
    ResetEvent(readEvent_);
    DWORD received = 0;
    if (!ReadFile(port_, bytes.data(), static_cast<DWORD>(bytes.size()), &received, &operation))
    {
        if (GetLastError() != ERROR_IO_PENDING)
            fail("ReadFile");
        if (WaitForSingleObject(readEvent_, 20) != WAIT_OBJECT_0)
        {
            CancelIoEx(port_, &operation);
            GetOverlappedResult(port_, &operation, &received, TRUE);
            throw std::runtime_error("Button feedback read timed out");
        }
        if (!GetOverlappedResult(port_, &operation, &received, FALSE))
            fail("Read feedback result");
    }
    return received;
}
void SerialPort::write(std::span<const std::uint8_t> frame)
{
    OVERLAPPED operation{};
    operation.hEvent = event_;
    ResetEvent(event_);
    DWORD written = 0;
    BOOL ok = WriteFile(port_, frame.data(), static_cast<DWORD>(frame.size()), &written, &operation);
    if (!ok)
    {
        if (GetLastError() != ERROR_IO_PENDING)
            fail("WriteFile");
        if (WaitForSingleObject(event_, 20) != WAIT_OBJECT_0)
        {
            CancelIoEx(port_, &operation);
            GetOverlappedResult(port_, &operation, &written, TRUE); // Keep frame/OVERLAPPED alive through cancellation.
            throw std::runtime_error("Serial write timed out; motion lease will expire");
        }
        if (!GetOverlappedResult(port_, &operation, &written, FALSE))
            fail("GetOverlappedResult");
    }
    if (written != frame.size())
        throw std::runtime_error("Partial serial frame; closing connection and letting motion expire");
}
} // namespace mousebox
