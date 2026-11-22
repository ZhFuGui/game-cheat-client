#pragma once
#include "application/extensions/contracts.hpp"

namespace mousebox::games::cs2::extensions
{
inline const application::extensions::Definition &coverDefinition()
{
    static const application::extensions::Definition value{"cs2.cover-detection", "cs2", "CS2 掩体检测",
                                                           "extension.cs2.cover-detection.enabled", 1};
    return value;
}
} // namespace mousebox::games::cs2::extensions
