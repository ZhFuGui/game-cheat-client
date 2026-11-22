#pragma once
#include <stdint.h>

namespace mousebox_firmware
{
enum class Counter : uint8_t
{
    Accepted,
    Rejected,
    Stops,
    Expired,
    Steps,
    Submitted,
    Invalid,
    Overflow,
    Discarded,
    Polls,
    Naks,
    TransferErrors,
    ControlOut,
    ControlIn,
    ControlErrors,
    Cancelled,
    Synthetic
};
// The production target selects this empty policy: no periodic logging,
// register probes, histograms, or runtime counter updates.
struct NoCounters
{
    void add(Counter, uint8_t = 0, uint16_t = 1) {}
    template <class Profile, class Clock> void report(uint8_t, uint16_t, const uint8_t *) {}
};
struct TrackingCounters : NoCounters
{
    uint32_t received[3] = {}, submitted[3] = {}, polls[3] = {}, naks[3] = {};
    uint32_t invalidReports = 0, overflow = 0, discarded = 0, transferErrors = 0, movingMouseReports = 0;
    uint32_t controlOut = 0, controlIn = 0, controlErrors = 0, cancelledControls = 0;
    uint16_t accepted = 0, rejected = 0, stops = 0, expired = 0, steps = 0;
    uint16_t syntheticReports = 0;
    void add(Counter event, uint8_t index = 0, uint16_t amount = 1)
    {
        switch (event)
        {
        case Counter::Accepted:
            accepted += amount;
            break;
        case Counter::Rejected:
            rejected += amount;
            break;
        case Counter::Stops:
            stops += amount;
            break;
        case Counter::Expired:
            expired += amount;
            break;
        case Counter::Steps:
            steps += amount;
            break;
        case Counter::Submitted:
            submitted[index] += amount;
            break;
        case Counter::Invalid:
            invalidReports += amount;
            break;
        case Counter::Overflow:
            overflow += amount;
            break;
        case Counter::Discarded:
            discarded += amount;
            break;
        case Counter::Polls:
            polls[index] += amount;
            break;
        case Counter::Naks:
            naks[index] += amount;
            break;
        case Counter::TransferErrors:
            transferErrors += amount;
            break;
        case Counter::ControlOut:
            controlOut += amount;
            break;
        case Counter::ControlIn:
            controlIn += amount;
            break;
        case Counter::ControlErrors:
            controlErrors += amount;
            break;
        case Counter::Cancelled:
            cancelledControls += amount;
            break;
        case Counter::Synthetic:
            syntheticReports += amount;
            break;
        }
    }
    template <class Profile, class Clock> void report(uint8_t endpoint, uint16_t length, const uint8_t *bytes)
    {
        ++received[Profile::index(endpoint)];
        if (endpoint == Profile::MouseEndpoint && Profile::valid(endpoint, length, bytes) && Profile::moved(bytes))
            ++movingMouseReports;
    }
};
} // namespace mousebox_firmware
