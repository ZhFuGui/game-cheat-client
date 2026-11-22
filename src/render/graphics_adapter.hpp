#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace mousebox::render
{
struct RenderAdapter
{
    std::string id;
    std::wstring name;
    std::uint64_t dedicatedBytes = 0, sharedBytes = 0;
    bool software = false, systemPerformanceOrder = false;
    bool operator==(const RenderAdapter &) const = default;
};
// DXGI inventory in high-performance preference order. No D3D device is
// created on the UI thread; actual compatibility is checked by the worker.
std::vector<RenderAdapter> enumerateRenderAdapters();
} // namespace mousebox::render
