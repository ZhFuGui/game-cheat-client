#pragma once
#include "domain/game.hpp"

namespace mousebox::games::valorant
{
// Register identity now. Runtime capabilities require a paired Reader contract
// and this game's own source, configuration and application adapters.
inline constexpr GameDefinition Definition{"valorant", "VALORANT", "unavailable", "reader_contract_pending", {}, {}};
} // namespace mousebox::games::valorant
