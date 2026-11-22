#pragma once
#include "games/cs2/extensions/head_damage/result.hpp"
#include <string>
namespace mousebox::games::cs2
{
std::wstring headDamageLabel(const extensions::HeadDamageResult *, const Snapshot &, const Player &, bool english);
}
