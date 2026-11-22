#pragma once
#include "runtime_service.hpp"
#include "application/storage/json_store.hpp"
#include "application/diagnostics/presentation_format.hpp"
#include "platform/windows/input.hpp"
#include <Windows.h>
#include <sstream>
#include <iomanip>

namespace mousebox::application::runtime_detail
{
inline Json ok()
{
    return {{"ok", true}};
}
inline void require(bool b, const char *text)
{
    if (!b)
        throw std::runtime_error(text);
}
inline std::filesystem::path pathOf(const Json &payload)
{
    const auto text = payload.at("path").get<std::string>();
    require(!text.empty() && text.size() <= 32768, "文件路径不合法");
    return storage::wide(text);
}
inline Json speedPresets(control::DeviceMotionLimits limits)
{
    Json result = Json::array();
    for (unsigned i = 0; i < 5; ++i)
    {
        const auto preset = static_cast<control::SpeedPreset>(i);
        const auto p = control::speedPolicy(preset, limits);
        result.push_back({{"value", i},
                          {"maximumCountsPerSecond", p.maximumCountsPerSecond},
                          {"acceleration", p.acceleration},
                          {"percent", control::speedFraction(preset) * 100},
                          {"intervalMs", p.intervalMs},
                          {"maximumStepCounts", p.maximumStepCounts}});
    }
    return result;
}
inline std::string unique()
{
    return std::to_string(GetTickCount64()) + "-" + std::to_string(randomSession());
}
inline std::string nowText()
{
    SYSTEMTIME time{};
    GetLocalTime(&time);
    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << time.wYear << '-' << std::setw(2) << time.wMonth << '-'
        << std::setw(2) << time.wDay << 'T' << std::setw(2) << time.wHour << ':' << std::setw(2)
        << time.wMinute << ':' << std::setw(2) << time.wSecond << '.' << std::setw(3) << time.wMilliseconds;
    return out.str();
}
using presentation::nullable;
using presentation::number;
} // namespace mousebox::application::runtime_detail
