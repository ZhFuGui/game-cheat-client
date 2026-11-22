#pragma once
#include "connection.hpp"
#include "games/pubg/model/control_frame.hpp"
#include "domain/reader_chain_metrics.hpp"
#include "application/ports/source_inspection.hpp"
#include <filesystem>
#include <functional>
#include <memory>
#include <stop_token>

namespace mousebox::reader
{
inline constexpr unsigned EntityChannel = 1, LocalChannel = 6;
// Application-facing adapter port. Frozen wire types and receiver state never
// cross this header; all currentness decisions are made before publication.
struct SourceState
{
    std::shared_ptr<const control::Frame> frame;
    bool ready = false, stateChanged = false;
    std::optional<double> roundTripMs;
    std::optional<std::uint64_t> receivedAtMs;
    std::optional<domain::ReaderChainSample> chainTiming;
    application::ports::SourcePacket inspection;
};
struct SourceOptions
{
    std::string host = "127.0.0.1", instance;
    unsigned short port = 13102;
    double hz = 30;
    std::function<void(unsigned, const SourceState &)> onState;
    std::function<void(const std::string &)> onLog;
    std::function<void(unsigned, ConnectionPhase, const std::string &)> onConnection;
    std::function<void(unsigned, std::uint64_t, std::uint64_t)> onTraffic;
};
void observeSource(const SourceOptions &, unsigned category, std::stop_token);
void bindSourceLocal(control::Frame &living, const control::Frame *local);
struct RecordedFrames
{
    control::Frame living, local;
};
RecordedFrames loadRecordedFrames(const std::filesystem::path &root);
} // namespace mousebox::reader
