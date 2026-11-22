#pragma once
#include "application/ports/game_module.hpp"
#include <memory>

namespace mousebox::application
{
class Backend;
}
namespace mousebox::desktop::composition
{
// The executable owns registrations. Core contracts never select a concrete game.
std::unique_ptr<application::ports::GameModule> createGameModule(std::string_view, application::Backend &);
} // namespace mousebox::desktop::composition
