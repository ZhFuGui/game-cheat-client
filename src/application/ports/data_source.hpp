#pragma once
#include <cstdint>
#include <string>
#include "source_inspection.hpp"

namespace mousebox::application::ports
{
struct SourceConnection
{
    std::string host;
    std::uint16_t port = 0;
    double hz = 0;
};
// The adapter owns protocol, categories, source clocks and currentness proofs.
// Optional inspection exposes opaque immutable read callbacks, never SDK state.
class DataSource
{
  public:
    virtual ~DataSource() = default;
    virtual void connect(const SourceConnection &) = 0;
    virtual void disconnect() = 0;
    virtual bool requested() const = 0;
    virtual SourcePackets inspectSources() const
    {
        return {};
    }
};
} // namespace mousebox::application::ports
