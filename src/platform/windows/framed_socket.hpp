#pragma once
#include <winsock2.h>
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <stop_token>
#include <string>
#include <string_view>

namespace mousebox::platform
{
struct ReceivedFrame
{
    std::string body;
    std::uint64_t tick = 0;
};
class Socket
{
  public:
    using TrafficObserver = std::function<void(std::uint64_t sent, std::uint64_t received)>;
    Socket(std::string_view ipv4, unsigned short port, std::stop_token, TrafficObserver = {});
    ~Socket();
    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;
    std::optional<ReceivedFrame> poll(unsigned maximum, unsigned progressTimeoutMs, std::stop_token);
    std::uint64_t send(std::string packet, unsigned timeoutMs, std::stop_token,
                       std::function<void(std::string &, std::uint64_t)> stamp = {});

  private:
    bool wait(bool write, std::stop_token);
    SOCKET socket_ = INVALID_SOCKET;
    TrafficObserver traffic_;
    std::string prefix_, body_;
    unsigned expected_ = 0;
    std::chrono::steady_clock::time_point progress_ = std::chrono::steady_clock::now();
};
} // namespace mousebox::platform
