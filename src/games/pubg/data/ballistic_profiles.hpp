#pragma once
#include "control/ballistics.hpp"
#include <filesystem>
#include <memory>
namespace mousebox::application
{
std::shared_ptr<const control::BallisticCatalog> loadBallisticProfiles(const std::filesystem::path &);
std::shared_ptr<const control::BallisticCatalog> defaultBallisticProfiles(
    const std::filesystem::path &executableDirectory);
} // namespace mousebox::application
