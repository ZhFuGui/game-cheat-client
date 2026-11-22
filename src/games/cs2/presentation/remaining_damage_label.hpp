#pragma once
#include "games/cs2/extensions/remaining_damage/result.hpp"
#include <string>
namespace mousebox::games::cs2
{
std::wstring remainingDamageLabel(const extensions::RemainingDamageResult *, const Snapshot &, const Player &,
                                  bool english);
}
