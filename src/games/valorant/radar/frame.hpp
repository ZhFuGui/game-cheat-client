#pragma once
#include "application/json.hpp"
#include "adapters/sources/valorant_reader/client.hpp"
namespace mousebox::games::valorant
{
application::Json readRadar(const sources::valorant_reader::Observation &, const application::Json &);
}
