#pragma once
#include "games/cs2/model/snapshot.hpp"
#include <string_view>
namespace mousebox::sources::cs2_reader
{
inline constexpr unsigned MaximumRecord = 16 * 1024 * 1024;
extern const std::string_view SchemaHash;
struct Envelope
{
    unsigned body = 0, availability = 0;
    std::uint64_t event = 0, frequency = 0, request = 0;
    std::optional<std::uint64_t> received, sent, statusEpoch;
    std::string instance, clockDomain, error;
    double captureHz = 0, publishHz = 0;
    bool full = false, clockExchange = false, readOnly = false;
    bool independentGroupTimestamps = false, verifiedBoneSemantics = false;
    bool recoilTimeState = false;
    bool mapIdentity = false;
    bool boneWorldPose = false;
    bool weaponAccuracyEvaluation = false;
    std::shared_ptr<const games::cs2::Snapshot> snapshot;
};
Envelope decode(std::string_view);
void validateSnapshotCapabilities(const Envelope &hello, const games::cs2::Snapshot &);
std::string subscribe(std::uint64_t request, double maximumHz);
std::string clockQuery(std::uint64_t request);
std::string record(std::string body);
} // namespace mousebox::sources::cs2_reader
