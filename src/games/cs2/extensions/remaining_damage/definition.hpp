#pragma once
#include "application/extensions/contracts.hpp"
namespace mousebox::games::cs2::extensions
{
inline const application::extensions::Definition &remainingDamageDefinition()
{
    static const application::extensions::Definition value{"cs2.remaining-damage", "cs2", "CS2 穿透余伤",
                                                           "extension.cs2.remaining-damage.enabled", 1};
    return value;
}
} // namespace mousebox::games::cs2::extensions
