#pragma once
#include "application/json.hpp"
namespace mousebox::games::cs2
{
struct Snapshot;
application::Json describeViewpoint(const Snapshot &);
} // namespace mousebox::games::cs2
