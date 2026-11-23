#pragma once
#include "codec.hpp"
#include "application/ports/data_source.hpp"
#include "domain/reader_chain_metrics.hpp"
#include "domain/source_sample.hpp"
#include "domain/source_receipt.hpp"
#include <functional>
#include <memory>

namespace mousebox::sources::valorant_reader
{
struct Channel
{
    std::shared_ptr<const StreamState> data;
    std::string instance, error, subscriptionError;
    bool connected = false, connecting = false, pending = false;
    double requestedHz = 60;
    std::optional<double> subscribedHz;
    std::uint64_t connection = 0, receivedAtMs = 0;
    unsigned framesPerSecond = 0;
    domain::ReaderChainSnapshot chain;
    domain::SourceSample sample;
    std::optional<domain::SourceReceipt> receipt;
};
struct Observation
{
    bool requested = false;
    std::uint64_t revision = 0;
    std::array<Channel, 3> channels;
    double upload = 0, download = 0;
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
    void setRates(std::array<double, 3>);
    Observation observe() const;
};
} // namespace mousebox::sources::valorant_reader
