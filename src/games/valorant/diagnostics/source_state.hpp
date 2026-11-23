#pragma once
#include "application/json.hpp"
#include "adapters/sources/valorant_reader/client.hpp"
namespace mousebox::games::valorant
{
application::Json describeSource(const sources::valorant_reader::Observation &);
application::Json readScene(const sources::valorant_reader::Observation &, const application::Json &);
} // namespace mousebox::games::valorant
