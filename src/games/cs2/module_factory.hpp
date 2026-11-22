#pragma once
#include "application/ports/game_module.hpp"
#include "application/ports/output_program.hpp"
#include "application/ports/scene_visibility.hpp"
#include "application/ports/shot_prediction.hpp"
#include "application/extensions/contracts.hpp"
#include "games/cs2/model/snapshot.hpp"
namespace mousebox::games::cs2
{
std::unique_ptr<application::ports::GameModule> makeModule(
    std::unique_ptr<application::ports::OutputBinding>,
    std::vector<std::unique_ptr<application::extensions::Extension<Snapshot>>> extensions = {},
    std::shared_ptr<application::ports::VisibilityProvider> geometry = {},
    std::shared_ptr<application::ports::BallisticsProvider> ballistics = {});
}
