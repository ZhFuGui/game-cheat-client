#pragma once
#include "application/json.hpp"
#include <cmath>
#include <stdexcept>
#include <string_view>
namespace mousebox::application
{
inline bool isApplicationValue(std::string_view key) noexcept
{
    return key == "dpi" || key == "renderAdapter";
}
inline void validateApplicationValue(std::string_view key, const Json &value)
{
    if (key == "dpi")
    {
        if (!value.is_number_integer() || value.get<std::int64_t>() < 100 || value.get<std::int64_t>() > 32000)
            throw std::invalid_argument("全局硬件DPI无效");
    }
    else if (key == "renderAdapter")
    {
        if (!value.is_string() || value.get_ref<const std::string &>().empty() ||
            value.get_ref<const std::string &>().size() > 512)
            throw std::invalid_argument("全局显卡选择无效");
    }
    else
        throw std::invalid_argument("未知全局参数");
}
} // namespace mousebox::application
