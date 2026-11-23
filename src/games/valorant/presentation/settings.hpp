#pragma once
#include "application/json.hpp"

namespace mousebox::games::valorant
{
struct VisualSettings
{
    bool self = true, allies = true, enemies = true, unknown = true, camera = true;
    double metersPerUnit = 0;
    std::string windowExecutable = "VALORANT-Win64-Shipping.exe";
};
VisualSettings visualSettings(const application::Json &);
// 0 unknown, 1 self, 2 ally, 3 enemy. This is a relationship, never an alive/visibility classification.
bool included(unsigned relationship, const VisualSettings &);
} // namespace mousebox::games::valorant
