#pragma once
#include "application/json.hpp"
namespace mousebox::sources::cs2_reader
{
struct Observation;
}
namespace mousebox::games::cs2
{
application::Json describeSnapshot(const sources::cs2_reader::Observation &, double metersPerUnit, bool diagnostics);
}
