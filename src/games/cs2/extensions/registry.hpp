#pragma once
#include "application/extensions/contracts.hpp"
#include "application/ports/scene_visibility.hpp"
#include "application/ports/shot_prediction.hpp"
#include "games/cs2/model/snapshot.hpp"

namespace mousebox::games::cs2::extensions
{
void addSettingDefaults(application::Json &);
bool permitsSetting(std::string_view) noexcept;
bool validateSetting(std::string_view, const application::Json &);
std::vector<std::unique_ptr<application::extensions::Extension<Snapshot>>> create(
    std::shared_ptr<application::ports::VisibilityProvider>,
    std::shared_ptr<application::ports::BallisticsProvider> = {});
} // namespace mousebox::games::cs2::extensions
