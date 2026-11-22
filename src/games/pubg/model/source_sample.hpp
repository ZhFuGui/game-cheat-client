#pragma once
#include "control_frame.hpp"
#include "domain/source_sample.hpp"

namespace mousebox::games::pubg
{
inline domain::SourceSample sourceSample(const control::Header &header)
{
    return {header.category == control::Category::Living ? 1u : 6u,
            header.current,
            header.reader,
            header.clockDomain,
            header.session,
            header.epoch,
            header.capture,
            header.mappingRevision,
            header.consumerConnection,
            header.sourceStartNs,
            header.sourceFinishNs,
            header.sourceClock};
}
} // namespace mousebox::games::pubg
