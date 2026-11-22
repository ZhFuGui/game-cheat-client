#pragma once
#include "application/json.hpp"
#include "control/movement/trajectory.hpp"
#include <string_view>
namespace mousebox::application
{
bool isTrajectorySetting(std::string_view) noexcept;
void addTrajectoryDefaults(Json &);
void validateTrajectorySetting(std::string_view, const Json &);
control::movement::TrajectorySettings readTrajectorySettings(const Json &);
} // namespace mousebox::application
