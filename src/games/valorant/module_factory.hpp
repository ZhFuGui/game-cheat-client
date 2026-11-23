#pragma once
#include "application/ports/game_module.hpp"
#include "application/ports/output_program.hpp"
namespace mousebox::games::valorant
{
std::unique_ptr<application::ports::GameModule> makeModule(std::unique_ptr<application::ports::OutputBinding>);
}
