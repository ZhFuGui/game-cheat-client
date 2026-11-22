#include "reader_metrics_presentation.hpp"
#include "application/diagnostics/reader_chain.hpp"
namespace mousebox::application
{
Json describeReaderMetrics(const ReaderMetricsSnapshot &s, bool gpuCompletionAvailable)
{
    Json channels = Json::array();
    for (const auto &c : s.channels)
        channels.push_back({{"category", c.category},
                            {"connection", std::to_string(c.connection)},
                            {"name", c.name},
                            {"status", c.status},
                            {"averageMs", c.averageMs ? Json(*c.averageMs) : Json{}},
                            {"uncertaintyMs", c.uncertaintyMs ? Json(*c.uncertaintyMs) : Json{}},
                            {"samples", c.samples},
                            {"receivedFramesPerSecond", c.receivedFramesPerSecond},
                            {"chain", describeReaderChain(c.chain)}});
    return {{"windowMs", 1000},
            {"uploadBytesPerSecond", s.uploadBytesPerSecond},
            {"downloadBytesPerSecond", s.downloadBytesPerSecond},
            {"gpuCompletionAvailable", gpuCompletionAvailable},
            {"channels", std::move(channels)}};
}
} // namespace mousebox::application
