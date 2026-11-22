#pragma once
#include <filesystem>

namespace mousebox::application::assets
{
enum class Kind
{
    MouseCapture,
    MouseModel,
    MouseReports
};
// New assets use application-wide storage; workspace documents own bindings.
// Existing absolute model/sample paths remain valid and are never relocated.
std::filesystem::path directory(const std::filesystem::path &dataRoot, Kind kind);
} // namespace mousebox::application::assets
