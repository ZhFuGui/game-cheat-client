#pragma once
#include "application/json.hpp"
#include "games/cs2/model/snapshot.hpp"
#include "games/cs2/hitboxes/assets.hpp"

namespace mousebox::games::cs2
{
application::Json describeHitboxes(const Snapshot *, hitboxes::Display);
} // namespace mousebox::games::cs2
