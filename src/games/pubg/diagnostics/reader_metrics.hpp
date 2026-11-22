#pragma once
#include "games/pubg/model/control_frame.hpp"
#include "domain/rolling_counter.hpp"
#include "domain/source_sample.hpp"
#include "domain/reader_chain_metrics.hpp"
#include <array>
#include <mutex>
#include <optional>
#include <span>

namespace mousebox::application
{
struct ReaderChannelMetrics
{
    unsigned category = 0;
    std::uint64_t connection = 0;
    std::string name, status;
    std::optional<double> averageMs, uncertaintyMs;
    std::uint64_t samples = 0;
    std::uint64_t receivedFramesPerSecond = 0;
    domain::ReaderChainSnapshot chain;
};
struct ReaderMetricsSnapshot
{
    std::uint64_t uploadBytesPerSecond = 0, downloadBytesPerSecond = 0;
    std::vector<ReaderChannelMetrics> channels;
};
// Separate bounded telemetry lock: network / GPU notifications never take the
// control or application-state lock. One-ms buckets retain the trailing second
// without limiting the number of frames or copying Reader payloads to the UI.
class ReaderMetrics
{
    struct Bucket
    {
        std::uint64_t time = 0, upload = 0, download = 0, count = 0;
        double delay = 0, uncertainty = 0;
    };
    using Window = std::array<Bucket, 1000>;
    struct Channel
    {
        unsigned category = 0;
        std::string name;
        std::uint64_t connection = 0, completedCapture = 0;
        std::uint64_t receivedCapture = 0;
        bool connected = false;
        std::optional<control::Header> current, scope;
        Window window{};
        RollingCounter received;
        domain::ReaderChainMetrics chain;
    };
    mutable std::mutex mutex_;
    Window traffic_{};
    std::vector<Channel> channels_;
    std::uint64_t nextConnection_ = 0;
    Channel *find(unsigned);
    static Bucket &bucket(Window &, std::uint64_t);

  public:
    void subscribe(std::vector<std::pair<unsigned, std::string>>);
    std::uint64_t begin(unsigned category);
    void observe(unsigned category, const control::Header *, std::uint64_t nowMs,
                 const domain::ReaderChainSample *chain = nullptr);
    void traffic(std::uint64_t nowMs, std::uint64_t upload, std::uint64_t download);
    void completed(std::span<const control::Header>, double completedAtMs);
    void completed(std::span<const domain::SourceSample>, double completedAtMs);
    ReaderMetricsSnapshot snapshot(std::uint64_t nowMs) const;
};
} // namespace mousebox::application
