#pragma once
#include "pose.hpp"
#include "games/cs2/model/snapshot.hpp"

namespace mousebox::games::cs2::hitboxes
{
Pose fromBody(const Snapshot &, const Body &);
std::optional<PosedHitbox> fromBody(const Snapshot &, const Body &, unsigned id);
bool retainsEligibility(const Snapshot &analyzed, const Snapshot &current);
} // namespace mousebox::games::cs2::hitboxes
