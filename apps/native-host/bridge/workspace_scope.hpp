#pragma once
#include "application/json.hpp"
#include <string_view>

namespace mousebox::desktop
{
inline bool workspaceScoped(std::string_view command, const application::Json& payload)
{
    if (command == "surface.hide"||command=="surface.configure"||command=="surface.minimize") return false;
    if (command == "settings.set")
    {
        const auto key = payload.value("key", std::string{});
        return key != "dpi" && key != "renderAdapter";
    }
    if (command == "settings.save" || command == "settings.import") return true;
    for (const auto prefix : {"reader.", "esp.", "calibration.", "learning.", "samples.", "model.",
                              "recording.", "algorithm.", "mouseReports.", "surface.", "game."})
        if (command.starts_with(prefix)) return true;
    return false;
}
}
