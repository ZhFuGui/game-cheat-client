#pragma once
#include "model.hpp"
#include "application/json.hpp"
#include <string_view>

namespace mousebox::games::valorant::aim
{
void addDefaults(application::Json &values);
bool isSetting(std::string_view) noexcept;
void validateSetting(std::string_view, const application::Json &);
Settings readSettings(const application::Json &);
} // namespace mousebox::games::valorant::aim
