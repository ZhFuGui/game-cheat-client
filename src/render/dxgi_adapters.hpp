#pragma once
#include "graphics_adapter.hpp"
#include <dxgi1_6.h>
#include <wrl/client.h>
namespace mousebox::render::detail
{
struct NativeAdapter
{
    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    RenderAdapter info;
    std::string fingerprint, luid;
};
std::vector<NativeAdapter> nativeAdapters();
} // namespace mousebox::render::detail
