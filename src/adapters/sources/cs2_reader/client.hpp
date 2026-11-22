#pragma once
#include "codec.hpp"
#include "application/ports/data_source.hpp"
#include "domain/source_sample.hpp"
#include "domain/reader_chain_metrics.hpp"
#include "domain/source_receipt.hpp"
#include <functional>
#include <span>
namespace mousebox::sources::cs2_reader
{
struct Metrics
{
    double upload = 0, download = 0;
    unsigned frames = 0, samples = 0;
    std::optional<double> averageMs, uncertaintyMs;
    bool clock = false;
    std::uint64_t connection = 0;
    domain::ReaderChainSnapshot chain;
};
struct Observation
{
    std::shared_ptr<const games::cs2::Snapshot> frame;
    bool requested = false, connected = false, connecting = false, current = false;
    bool verifiedBoneSemantics = false;
    bool boneWorldPose = false;
    bool recoilTimeState = false;
    bool weaponAccuracyEvaluation = false;
    std::string error;
    std::uint64_t revision = 0, receivedAtMs = 0;
    std::uint64_t observedAtMs = 0;
    Metrics metrics;
    double requestedHz = 30;
    std::optional<double> subscribedHz, captureHz;
    bool subscriptionPending = false;
    std::string subscriptionError;
    std::optional<domain::SourceSample> sample;
    std::optional<domain::SourceReceipt> receipt;
};
class Client final : public application::ports::DataSource
{
    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    Client();
    ~Client();
    void connect(const application::ports::SourceConnection &) override;
    void disconnect() override;
    bool requested() const override;
    application::ports::SourcePackets inspectSources() const override;
    void setNotifier(std::function<void()>);
    void setMaximumAge(unsigned milliseconds);
    void setFrequency(double hz);
    Observation observe() const;
    void completed(std::span<const domain::SourceSample>, double completedAtMs);
};
} // namespace mousebox::sources::cs2_reader
