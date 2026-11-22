#pragma once
#include "font_resource.h"
#include <Windows.h>
#include <cstddef>
#include <span>

namespace mousebox::desktop
{
// The executable owns these bytes for the lifetime of any render worker.
inline std::span<const std::byte> embeddedFont() noexcept
{
    const auto module = GetModuleHandleW(nullptr);
    const auto resource = FindResourceW(module, MAKEINTRESOURCEW(MOUSEBOX_PRIMARY_FONT), RT_RCDATA);
    if (!resource)
        return {};
    const auto loaded = LoadResource(module, resource);
    if (const auto data = loaded ? LockResource(loaded) : nullptr)
        return {static_cast<const std::byte *>(data), SizeofResource(module, resource)};
    return {};
}
} // namespace mousebox::desktop
