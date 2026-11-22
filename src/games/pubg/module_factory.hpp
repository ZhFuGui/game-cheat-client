#pragma once
#include "application/ports/game_module.hpp"
#include <memory>

namespace mousebox::application
{
class Backend;
}
namespace mousebox::games::pubg
{
// Existing typed runtime name remains source-compatible during this migration;
// implementation and all PUBG state now live in games/pubg/runtime.
using Runtime = application::Backend;
std::unique_ptr<application::ports::GameModule> makeModule(Runtime &);
} // namespace mousebox::games::pubg
