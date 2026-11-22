#pragma once
#include "connection.hpp"
#include <stop_token>
#include <functional>
#include <cstdint>
#include <string>
namespace mousebox::reader
{
class Session;
struct Options
{
    std::string host = "127.0.0.1";
    unsigned short port = 13102;
    double hz = 30;
    unsigned seconds = 0;
    unsigned reconnectLivingAfterSeconds = 0;
    std::string instance;
    std::string evidenceDirectory;
    bool probes = true, declareClock = true;
    // Called synchronously on either category's worker. Session is borrowed only for this call;
    // nullptr immediately revokes this category. The callback must be thread-safe and bounded.
    std::function<void(unsigned, const Session *)> onState;
    std::function<void(const std::string &)> onLog;
    std::function<void(unsigned, ConnectionPhase, const std::string &)> onConnection;
    // Successful stream bytes only, including record framing and control records.
    // It must remain bounded; no payloads or per-packet Web events.
    std::function<void(unsigned, std::uint64_t, std::uint64_t)> onTraffic;
};
void observe(const Options &, unsigned category, std::stop_token);
void report(const std::string &);
} // namespace mousebox::reader
