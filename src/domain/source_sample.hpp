#pragma once
#include "domain/source_clock.hpp"
#include <cstdint>
#include <optional>
#include <string>

namespace mousebox::domain
{
// Minimal completion provenance. No entity layout, game identity or SDK type is
// needed to pair a rendered batch with its actual consumer connection and clock.
struct SourceSample
{
    unsigned channel = 0;
    bool current = false;
    std::string reader, clockDomain;
    std::uint64_t session = 0, epoch = 0, capture = 0, mappingRevision = 0, consumerConnection = 0;
    std::int64_t sourceStartNs = 0, sourceFinishNs = 0;
    std::optional<control::SourceClockEstimate> sourceClock;
};
} // namespace mousebox::domain
