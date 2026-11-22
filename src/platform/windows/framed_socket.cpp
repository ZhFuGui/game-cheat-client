#include "platform/windows/framed_socket.hpp"
#include "platform/windows/qpc.hpp"
#include <ws2tcpip.h>
#include <algorithm>
#include <array>
#include <stdexcept>

namespace mousebox::platform
{
namespace
{
using Clock = std::chrono::steady_clock;
void check(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
Socket::Socket(std::string_view host, unsigned short port, std::stop_token stop, TrafficObserver traffic)
    : traffic_(std::move(traffic))
{
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    check(socket_ != INVALID_SOCKET, "TCP socket failed");
    try
    {
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        check(InetPtonA(AF_INET, std::string(host).c_str(), &address.sin_addr) == 1,
              "Reader host must be an IPv4 address");
        u_long nonblocking = 1;
        int nodelay = 1;
        check(ioctlsocket(socket_, FIONBIO, &nonblocking) == 0 &&
                  setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&nodelay),
                             sizeof(nodelay)) == 0,
              "TCP setup failed");
        if (connect(socket_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == SOCKET_ERROR)
        {
            check(WSAGetLastError() == WSAEWOULDBLOCK, "TCP connect failed");
            const auto deadline = Clock::now() + std::chrono::seconds(3);
            while (!wait(true, stop))
                check(Clock::now() < deadline, "TCP connect timeout");
            int issue = 0, size = sizeof(issue);
            check(getsockopt(socket_, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&issue), &size) == 0 && !issue,
                  "TCP connection refused");
        }
    }
    catch (...)
    {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
        throw;
    }
}
Socket::~Socket()
{
    if (socket_ != INVALID_SOCKET)
        closesocket(socket_);
}
bool Socket::wait(bool write, std::stop_token stop)
{
    check(!stop.stop_requested(), "Reader cancelled");
    fd_set data, error;
    FD_ZERO(&data);
    FD_ZERO(&error);
    FD_SET(socket_, &data);
    FD_SET(socket_, &error);
    timeval timeout{0, 50000};
    const int result = select(0, write ? nullptr : &data, write ? &data : nullptr, &error, &timeout);
    check(result != SOCKET_ERROR && !FD_ISSET(socket_, &error), "TCP readiness failed");
    return result > 0;
}
std::optional<ReceivedFrame> Socket::poll(unsigned maximum, unsigned timeoutMs, std::stop_token stop)
{
    const auto partial = !prefix_.empty();
    if (partial && Clock::now() - progress_ > std::chrono::milliseconds(timeoutMs))
        throw std::runtime_error("TCP partial record stalled");
    if (!wait(false, stop))
        return {};
    std::array<char, 8192> bytes{};
    const auto missing = prefix_.size() < 4 ? 4 - prefix_.size() : static_cast<std::size_t>(expected_) - body_.size();
    const int count = recv(socket_, bytes.data(), static_cast<int>(std::min(missing, bytes.size())), 0);
    if (count == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
        return {};
    check(count > 0, "TCP disconnected");
    if (traffic_)
        traffic_(0, static_cast<std::uint64_t>(count));
    progress_ = Clock::now();
    if (prefix_.size() < 4)
    {
        prefix_.append(bytes.data(), static_cast<std::size_t>(count));
        if (prefix_.size() == 4)
        {
            expected_ = 0;
            for (const unsigned char byte : prefix_)
                expected_ = (expected_ << 8) | byte;
            check(expected_ && expected_ <= maximum, "TCP record length exceeds negotiated bound");
            body_.reserve(expected_);
        }
    }
    else
        body_.append(bytes.data(), static_cast<std::size_t>(count));
    if (prefix_.size() == 4 && body_.size() == expected_)
    {
        const auto tick = readQpcTicks(); // Complete body has arrived; decode has not started.
        ReceivedFrame frame{std::move(body_), tick};
        prefix_.clear();
        body_.clear();
        expected_ = 0;
        return frame;
    }
    return {};
}
std::uint64_t Socket::send(std::string packet, unsigned timeoutMs, std::stop_token stop,
                           std::function<void(std::string &, std::uint64_t)> stamp)
{
    std::size_t written = 0;
    std::uint64_t t0 = 0;
    bool stamped = false;
    auto progress = Clock::now();
    while (written < packet.size())
    {
        check(Clock::now() - progress <= std::chrono::milliseconds(timeoutMs), "TCP write stalled");
        if (!wait(true, stop))
            continue;
        if (!stamped)
        {
            t0 = readQpcTicks();
            if (stamp)
                stamp(packet, t0);
            stamped = true;
        }
        const int count = ::send(socket_, packet.data() + written, static_cast<int>(packet.size() - written), 0);
        if (count == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
            continue;
        check(count > 0, "TCP write failed");
        if (traffic_)
            traffic_(static_cast<std::uint64_t>(count), 0);
        written += static_cast<std::size_t>(count);
        progress = Clock::now();
    }
    return t0;
}
} // namespace mousebox::platform
