#include "dxgi_adapters.hpp"
#include "directx_error.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <unordered_map>
namespace mousebox::render::detail
{
using Microsoft::WRL::ComPtr;
std::vector<NativeAdapter> nativeAdapters()
{
    ComPtr<IDXGIFactory1> factory;
    check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "Enumerate graphics factory");
    ComPtr<IDXGIFactory6> preferred;
    factory.As(&preferred);
    std::vector<NativeAdapter> result;
    for (UINT i = 0;; ++i)
    {
        ComPtr<IDXGIAdapter1> adapter;
        const auto hr = preferred ? preferred->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                                          IID_PPV_ARGS(&adapter))
                                  : factory->EnumAdapters1(i, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;
        check(hr, "Enumerate graphics adapter");
        DXGI_ADAPTER_DESC1 desc{};
        check(adapter->GetDesc1(&desc), "Graphics adapter description");
        std::ostringstream key, luid;
        key << "gpu:" << std::hex << std::setfill('0') << std::setw(8) << desc.VendorId << ':' << std::setw(8)
            << desc.DeviceId << ':' << std::setw(8) << desc.SubSysId << ':' << std::setw(8) << desc.Revision;
        luid << std::hex << std::setfill('0') << std::setw(8) << static_cast<std::uint32_t>(desc.AdapterLuid.HighPart)
             << std::setw(8) << desc.AdapterLuid.LowPart;
        result.push_back({adapter,
                          {key.str(), desc.Description, desc.DedicatedVideoMemory, desc.SharedSystemMemory,
                           (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0, preferred != nullptr},
                          key.str(),
                          luid.str()});
    }
    // A unique PCI fingerprint survives a reboot. Identical cards must retain
    // their distinct LUIDs rather than silently choosing a different device.
    std::unordered_map<std::string, unsigned> counts;
    for (const auto &item : result)
        ++counts[item.fingerprint];
    for (auto &item : result)
        if (counts[item.fingerprint] > 1)
            item.info.id += ':' + item.luid;
    if (!preferred)
        std::stable_sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
            if (a.info.software != b.info.software)
                return !a.info.software;
            return a.info.dedicatedBytes > b.info.dedicatedBytes;
        });
    return result;
}
} // namespace mousebox::render::detail
namespace mousebox::render
{
std::vector<RenderAdapter> enumerateRenderAdapters()
{
    std::vector<RenderAdapter> result;
    for (const auto &item : detail::nativeAdapters())
        result.push_back(item.info);
    return result;
}
} // namespace mousebox::render
