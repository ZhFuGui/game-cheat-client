#pragma once
#include "application/json.hpp"
#include "games/cs2/model/snapshot.hpp"

namespace mousebox::games::cs2
{
application::Json describeMap(const std::optional<MapIdentity> &);
}
