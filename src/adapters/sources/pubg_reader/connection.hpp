#pragma once

namespace mousebox::reader
{
enum class ConnectionPhase
{
    Connecting,
    Connected,
    Error,
    Closed
};
} // namespace mousebox::reader
