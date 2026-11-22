#pragma once
#include "domain/source_clock.hpp"
#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>

namespace mousebox::domain
{
struct ReaderChainSample
{
    std::string reader, clockDomain;
    std::uint64_t epoch = 0, capture = 0;
    std::optional<std::int64_t> startedNs, readFinishedNs, sendStartedNs;
    std::optional<control::SourceClockEstimate> clock;
    double receivedAtMs = 0;
};
struct ReaderStageAverage
{
    std::optional<double> averageMs, uncertaintyMs;
    std::uint64_t samples = 0;
    std::string status = "waiting-data";
};
struct ReaderChainSnapshot
{
    ReaderStageAverage read, prepare, transport;
};
// One instance per actual TCP connection, protected by its owner's telemetry
// lock. The receive boundary precedes decoding and is independent of drawing.
class ReaderChainMetrics
{
    struct Value
    {
        std::uint64_t count = 0;
        double sum = 0, uncertainty = 0;
    };
    struct Bucket
    {
        std::uint64_t at = 0;
        std::array<Value, 3> values;
    };
    std::deque<Bucket> buckets_;
    std::optional<ReaderChainSample> previous_;
    std::array<std::string, 3> reasons_{"waiting-data", "waiting-data", "waiting-data"};

  public:
    void reset();
    void observe(const ReaderChainSample &);
    ReaderChainSnapshot snapshot(std::uint64_t nowMs) const;
};
} // namespace mousebox::domain
