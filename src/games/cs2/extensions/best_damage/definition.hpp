#pragma once
#include "application/extensions/contracts.hpp"
namespace mousebox::games::cs2::extensions
{
inline const application::extensions::Definition &bestDamageDefinition()
{
    static const application::extensions::Definition value{"cs2.best-damage", "cs2", "CS2 最高伤害瞄准点",
                                                           "extension.cs2.best-damage.enabled", 1};
    return value;
}
} // namespace mousebox::games::cs2::extensions
