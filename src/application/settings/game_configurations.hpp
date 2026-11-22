#pragma once
#include "application/ports/game_configuration.hpp"
#include <span>

namespace mousebox::application
{
// Storage needs only stable game identity and the canonical configuration name.
// Catalog capabilities and concrete adapters stay in the composition entry point.
struct AvailableGameConfiguration
{
    std::string_view id, name;
};
std::span<const AvailableGameConfiguration> availableGameConfigurations();
bool isAvailableGameConfiguration(std::string_view gameId);
const ports::GameConfiguration &gameConfiguration(std::string_view gameId);
} // namespace mousebox::application
