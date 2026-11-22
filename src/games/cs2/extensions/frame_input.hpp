#pragma once
#include "application/extensions/contracts.hpp"
#include "games/cs2/model/snapshot.hpp"

namespace mousebox::games::cs2::extensions
{
inline application::extensions::Scope scope(const Snapshot &f)
{
    application::extensions::Scope s;
    s.source = f.instance;
    s.clock = f.clockDomain;
    s.epoch = f.epoch;
    s.process = f.processId;
    if (f.map)
    {
        s.map = f.map->name;
        s.mapResource = f.map->resourcePath;
        s.mapGeneration = f.map->generation;
    }
    return s;
}
inline application::extensions::Input<Snapshot> input(std::shared_ptr<const Snapshot> f,
                                                      std::optional<domain::SourceSample> sample,
                                                      std::uint64_t revision, std::uint64_t at,
                                                      std::optional<domain::SourceReceipt> receipt = {})
{
    application::extensions::Input<Snapshot> result;
    if (f)
    {
        result.scope = scope(*f);
        result.capture = f->sequence;
        result.receipt = std::move(receipt);
    }
    result.frame = std::move(f);
    result.sample = std::move(sample);
    result.revision = revision;
    result.observedAtMs = at;
    return result;
}
} // namespace mousebox::games::cs2::extensions
