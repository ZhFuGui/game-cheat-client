#pragma once
#include "application/json.hpp"
#include <filesystem>
#include <string>

namespace mousebox::application::workspace_storage
{
inline constexpr auto MaximumDocument = 4 * 1024 * 1024;
bool validId(const std::string &id);
std::filesystem::path settingsPath(const std::filesystem::path &root, const std::string &id);
void save(const std::filesystem::path &path, const Json &value);
// Both documents are restored before a prepared journal is removed. A committed
// journal only needs cleanup; it must never roll back the published documents.
void recoverTransaction(const std::filesystem::path &root);
void commitConfiguration(const std::filesystem::path &root, const std::string &id, Json &application,
                         const Json &previousDocument, const Json &nextApplication, const Json &nextDocument);
} // namespace mousebox::application::workspace_storage
