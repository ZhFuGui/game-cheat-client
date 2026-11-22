#pragma once
#include "application/ports/scene_visibility.hpp"
#include "application/ports/shot_prediction.hpp"
#include "platform/windows/dynamic_library.hpp"
#include <cs2-external-autowall/types.h>
#include <filesystem>

namespace mousebox::adapters::traceline::detail
{
class ProfilePackage;
std::shared_ptr<const ProfilePackage> loadProfiles(std::shared_ptr<const platform::DynamicLibrary>,
                                                   const std::filesystem::path &);
std::shared_ptr<const application::ports::BallisticsScene> bindBallistics(
    std::shared_ptr<const ProfilePackage>, cs2tl_scene *, std::shared_ptr<const application::ports::VisibilityScene>);
} // namespace mousebox::adapters::traceline::detail
