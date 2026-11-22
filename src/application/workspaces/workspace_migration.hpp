#pragma once
#include "application/json.hpp"
#include <filesystem>
#include <optional>

namespace mousebox::application::workspace_migration
{
// Prepare all provisional documents and verified backups first. The caller owns
// publication of workspace-index.json, the sole index commit point.
Json prepareLegacyConfiguration(const std::filesystem::path &root, const std::optional<Json> &imported,
                                Json &application);
Json prepareGameConfigurationIndex(const std::filesystem::path &root, const Json &index);
} // namespace mousebox::application::workspace_migration
