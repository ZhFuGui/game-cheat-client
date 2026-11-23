#pragma once
#include "settings.hpp"
#include "adapters/sources/valorant_reader/client.hpp"
#include "application/ports/game_module.hpp"

namespace mousebox::games::valorant
{
application::ports::PresentationFrame makeSubmission(sources::valorant_reader::Observation,
                                                     const application::ports::PresentationOptions &,
                                                     const VisualSettings &, std::uint64_t revision);
} // namespace mousebox::games::valorant
