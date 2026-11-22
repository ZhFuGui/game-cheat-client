#pragma once
#include "application/json.hpp"
namespace mousebox::games::cs2
{
struct Snapshot;
application::Json describeAccuracy(const Snapshot &, bool capability);
} // namespace mousebox::games::cs2
