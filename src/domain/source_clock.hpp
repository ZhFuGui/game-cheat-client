#pragma once
#include <cstdint>

namespace mousebox::control
{
// A bounded clock-domain estimate, never an exact one-way latency or device ACK.
struct SourceClockEstimate
{
    std::int64_t sourceAnchorNs = 0;
    double localAnchorMs = 0, uncertaintyMs = 0;
    std::uint64_t calibratedAtMs = 0;
    bool operator==(const SourceClockEstimate &) const = default;
};
} // namespace mousebox::control
