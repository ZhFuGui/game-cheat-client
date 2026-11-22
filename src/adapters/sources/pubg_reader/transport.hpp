#pragma once
#include "platform/windows/framed_socket.hpp"
#include <stdexcept>
namespace mousebox::reader
{
using ReceivedFrame = platform::ReceivedFrame;
class Socket final : public platform::Socket
{
  public:
    using platform::Socket::Socket;
    std::uint64_t send(std::string packet, unsigned timeoutMs, std::stop_token stop, bool probe = false)
    {
        if (!probe)
        {
            platform::Socket::send(std::move(packet), timeoutMs, stop);
            return 0;
        }
        return platform::Socket::send(std::move(packet), timeoutMs, stop, [](std::string &bytes, std::uint64_t tick) {
            if (bytes.size() < 13 || static_cast<unsigned char>(bytes[bytes.size() - 9]) != 0x11)
                throw std::runtime_error("Clock probe fixed64 placeholder missing");
            for (unsigned i = 0; i < 8; ++i)
                bytes[bytes.size() - 8 + i] = static_cast<char>(tick >> (i * 8));
        });
    }
};
} // namespace mousebox::reader
