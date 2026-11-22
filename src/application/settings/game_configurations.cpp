#include "game_configurations.hpp"
#include "games/pubg/settings/application_adapter.hpp"
#include "games/cs2/settings/configuration.hpp"
#include "games/registry.hpp"
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace mousebox::application
{
std::span<const AvailableGameConfiguration> availableGameConfigurations()
{
    static const auto configurations = [] {
        std::vector<AvailableGameConfiguration> result;
        for (const auto &game : games::gameDefinitions())
            if (game.status == "available")
                result.push_back({game.id, game.name});
        return result;
    }();
    return configurations;
}
bool isAvailableGameConfiguration(std::string_view gameId)
{
    const auto configurations = availableGameConfigurations();
    return std::any_of(configurations.begin(), configurations.end(),
                       [&](const auto &configuration) { return configuration.id == gameId; });
}
const ports::GameConfiguration &gameConfiguration(std::string_view id)
{
    const auto &pubg = games::pubg::settings::configuration();
    if (id == pubg.gameId())
        return pubg;
    if (id == "cs2")
        return games::cs2::configuration();
    throw std::runtime_error("此游戏没有已注册的配置适配器");
}
} // namespace mousebox::application
