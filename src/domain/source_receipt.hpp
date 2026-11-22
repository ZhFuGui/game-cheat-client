#pragma once
#include <cstdint>
#include <string>
namespace mousebox::domain
{
// Consumer-local provenance, independent of Reader clock calibration and control
// qualification. The timestamp is captured at complete TCP record reception.
struct SourceReceipt
{
    unsigned channel = 0;
    std::string reader;
    std::uint64_t session = 0, epoch = 0, capture = 0, connection = 0;
    double receivedAtMs = 0;
};
} // namespace mousebox::domain
