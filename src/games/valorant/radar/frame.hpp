#pragma once
#include "application/json.hpp"
#include "adapters/sources/valorant_reader/client.hpp"
#include "games/valorant/presentation/settings.hpp"
namespace mousebox::games::valorant
{
application::Json readRadar(const sources::valorant_reader::Observation &, const application::Json &,
                            const VisualSettings & = {});
}
