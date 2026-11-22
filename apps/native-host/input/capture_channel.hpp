#pragma once
#include "application/json.hpp"
#include <Windows.h>
#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace mousebox::desktop::capture
{
using application::Json;
inline constexpr std::size_t MaximumPayload = 64 * 1024 * 1024;
class Handle
{
    HANDLE value_ = nullptr;

  public:
    explicit Handle(HANDLE value = nullptr) : value_(value) {}
    ~Handle();
    Handle(const Handle &) = delete;
    Handle &operator=(const Handle &) = delete;
    Handle(Handle &&) noexcept;
    Handle &operator=(Handle &&) noexcept;
    HANDLE get() const
    {
        return value_;
    }
    explicit operator bool() const
    {
        return value_ && value_ != INVALID_HANDLE_VALUE;
    }
};
struct Frame
{
    Json metadata = Json::object();
    std::vector<std::byte> bytes;
};
std::wstring pipeName(DWORD parentPid, const std::string &token, const wchar_t *suffix);
Handle createServer(const std::wstring &);
void acceptClient(HANDLE pipe, HANDLE cancel, const std::filesystem::path &expectedExecutable);
Handle connect(const std::wstring &, HANDLE cancel, DWORD expectedServer);
Frame read(HANDLE pipe, HANDLE cancel, DWORD timeout = INFINITE);
void write(HANDLE pipe, HANDLE cancel, const Frame &, DWORD timeout = 3000);
void validateFrameSizes(std::uint32_t metadata, std::uint32_t payload);
} // namespace mousebox::desktop::capture
