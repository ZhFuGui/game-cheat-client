#pragma once
#include "application/extensions/contracts.hpp"
namespace mousebox::games::cs2::extensions
{
inline const application::extensions::Definition &headDamageDefinition()
{
    static const application::extensions::Definition value{"cs2.head-damage", "cs2", "CS2 预计头部伤害",
                                                           "extension.cs2.head-damage.enabled", 1};
    return value;
}
} // namespace mousebox::games::cs2::extensions
