#include "capture_channel.hpp"
#include <TlHelp32.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <stdexcept>
#include <utility>

namespace mousebox::desktop::capture
{
namespace
{
constexpr std::uint32_t Magic = 0x3143424d;
struct Header
{
    std::uint32_t magic = Magic, version = 1, metadata = 0, payload = 0;
};
void check(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
DWORD complete(HANDLE pipe, OVERLAPPED &operation, HANDLE cancel, DWORD timeout)
{
    const HANDLE events[]{operation.hEvent, cancel};
    const auto result = WaitForMultipleObjects(2, events, FALSE, timeout);
    if (result != WAIT_OBJECT_0)
    {
        CancelIoEx(pipe, &operation);
        DWORD ignored = 0;
        GetOverlappedResult(pipe, &operation, &ignored, TRUE);
        throw std::runtime_error(result == WAIT_TIMEOUT ? "原生采集通道超时" : "原生采集通道已关闭");
    }
    DWORD transferred = 0;
    check(GetOverlappedResult(pipe, &operation, &transferred, FALSE) != FALSE, "原生采集通道已断开");
    return transferred;
}
void transfer(HANDLE pipe, HANDLE cancel, void *data, std::size_t size, bool writing, DWORD timeout)
{
    auto *position = static_cast<std::byte *>(data);
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout == INFINITE ? 0 : timeout);
    while (size)
    {
        check(WaitForSingleObject(cancel, 0) != WAIT_OBJECT_0, "原生采集通道已关闭");
        Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        check(static_cast<bool>(event), "原生采集等待事件创建失败");
        OVERLAPPED operation{};
        operation.hEvent = event.get();
        const auto chunk = static_cast<DWORD>(std::min<std::size_t>(size, 1024 * 1024));
        DWORD count = 0;
        const auto started = writing ? WriteFile(pipe, position, chunk, &count, &operation)
                                     : ReadFile(pipe, position, chunk, &count, &operation);
        if (!started)
        {
            check(GetLastError() == ERROR_IO_PENDING, "原生采集通道已断开");
            DWORD remaining = INFINITE;
            if (timeout != INFINITE)
            {
                const auto ms =
                    std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now())
                        .count();
                remaining = ms > 0 ? static_cast<DWORD>(ms) : 0;
            }
            count = complete(pipe, operation, cancel, remaining);
        }
        check(count != 0, "原生采集通道意外结束");
        size -= count;
        position += count;
    }
}
bool approvedClient(DWORD pid, const std::filesystem::path &executable)
{
    if (pid == GetCurrentProcessId())
        return true; // Same-process offline pipe tests; no input registration.
    Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!snapshot)
        return false;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    bool child = false;
    for (BOOL found = Process32FirstW(snapshot.get(), &entry); found; found = Process32NextW(snapshot.get(), &entry))
        if (entry.th32ProcessID == pid)
        {
            child = entry.th32ParentProcessID == GetCurrentProcessId();
            break;
        }
    if (!child)
        return false;
    Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
    std::array<wchar_t, 32768> path{};
    DWORD length = static_cast<DWORD>(path.size());
    if (!process || !QueryFullProcessImageNameW(process.get(), 0, path.data(), &length))
        return false;
    std::error_code error;
    return std::filesystem::equivalent(std::filesystem::path(std::wstring(path.data(), length)), executable, error) &&
           !error;
}
} // namespace
Handle::~Handle()
{
    if (*this)
        CloseHandle(value_);
}
Handle::Handle(Handle &&other) noexcept : value_(std::exchange(other.value_, nullptr)) {}
Handle &Handle::operator=(Handle &&other) noexcept
{
    if (this != &other)
    {
        if (*this)
            CloseHandle(value_);
        value_ = std::exchange(other.value_, nullptr);
    }
    return *this;
}
std::wstring pipeName(DWORD pid, const std::string &token, const wchar_t *suffix)
{
    check(pid && token.size() == 36 && token.find_first_not_of("0123456789abcdef-") == std::string::npos,
          "原生采集端点标识不合法");
    return L"\\\\.\\pipe\\MouseboxCapture-" + std::to_wstring(pid) + L"-" + std::wstring(token.begin(), token.end()) +
           suffix;
}
Handle createServer(const std::wstring &name)
{
    Handle pipe(CreateNamedPipeW(name.c_str(),
                                 PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                                 PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1,
                                 64 * 1024, 64 * 1024, 0, nullptr));
    check(static_cast<bool>(pipe), "原生采集端点创建失败");
    return pipe;
}
void acceptClient(HANDLE pipe, HANDLE cancel, const std::filesystem::path &executable)
{
    Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    check(static_cast<bool>(event), "原生采集等待事件创建失败");
    OVERLAPPED operation{};
    operation.hEvent = event.get();
    if (!ConnectNamedPipe(pipe, &operation))
    {
        const auto error = GetLastError();
        if (error == ERROR_IO_PENDING)
            complete(pipe, operation, cancel, INFINITE);
        else
            check(error == ERROR_PIPE_CONNECTED, "原生采集端点连接失败");
    }
    ULONG pid = 0;
    check(GetNamedPipeClientProcessId(pipe, &pid) && approvedClient(pid, executable), "原生采集客户端身份不符");
}
Handle connect(const std::wstring &name, HANDLE cancel, DWORD expectedServer)
{
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    do
    {
        Handle pipe(CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                FILE_FLAG_OVERLAPPED, nullptr));
        if (pipe)
        {
            ULONG pid = 0;
            check(GetNamedPipeServerProcessId(pipe.get(), &pid) && pid == expectedServer, "原生采集服务端身份不符");
            return pipe;
        }
        check(GetLastError() == ERROR_PIPE_BUSY || GetLastError() == ERROR_FILE_NOT_FOUND, "无法打开原生采集端点");
    } while (WaitForSingleObject(cancel, 10) != WAIT_OBJECT_0 && std::chrono::steady_clock::now() < until);
    throw std::runtime_error("原生采集端点不可用");
}
void validateFrameSizes(std::uint32_t metadata, std::uint32_t payload)
{
    check(metadata && metadata <= 64 * 1024 && payload <= MaximumPayload, "原生采集帧长度超限");
}
Frame read(HANDLE pipe, HANDLE cancel, DWORD timeout)
{
    Header header;
    transfer(pipe, cancel, &header, sizeof(header), false, timeout);
    check(header.magic == Magic && header.version == 1, "原生采集帧版本不匹配");
    validateFrameSizes(header.metadata, header.payload);
    std::string text(header.metadata, '\0');
    Frame frame;
    frame.bytes.resize(header.payload);
    transfer(pipe, cancel, text.data(), text.size(), false, timeout);
    transfer(pipe, cancel, frame.bytes.data(), frame.bytes.size(), false, timeout);
    frame.metadata = Json::parse(text);
    check(frame.metadata.is_object(), "原生采集帧头不合法");
    return frame;
}
void write(HANDLE pipe, HANDLE cancel, const Frame &frame, DWORD timeout)
{
    auto text = frame.metadata.dump();
    check(text.size() <= 64 * 1024 && frame.bytes.size() <= MaximumPayload, "原生采集帧长度超限");
    Header header{Magic, 1, static_cast<std::uint32_t>(text.size()), static_cast<std::uint32_t>(frame.bytes.size())};
    transfer(pipe, cancel, &header, sizeof(header), true, timeout);
    transfer(pipe, cancel, text.data(), text.size(), true, timeout);
    transfer(pipe, cancel, const_cast<std::byte *>(frame.bytes.data()), frame.bytes.size(), true, timeout);
}
} // namespace mousebox::desktop::capture
