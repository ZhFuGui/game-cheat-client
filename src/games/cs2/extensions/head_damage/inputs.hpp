#pragma once
#include "games/cs2/extensions/remaining_damage/inputs.hpp"
#include "application/ports/shot_prediction.hpp"
namespace mousebox::games::cs2::extensions
{
std::optional<application::ports::PlayerDamageTarget> damageTarget(const Player &);
std::optional<Vec3> headImpact(const Snapshot &, const Body &, Vec3 origin);
} // namespace mousebox::games::cs2::extensions
