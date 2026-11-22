#pragma once
#include <string_view>
namespace mousebox::reader
{
// Descriptor of the formal decoder; SDK import and MSBuild pin checks must agree.
inline constexpr std::string_view ReaderContractDescriptorSha256 =
    "4e5f26fed497f86a37d686df443be54d42dcc5e1eeeae79143a19f209e5f6f92";
} // namespace mousebox::reader
