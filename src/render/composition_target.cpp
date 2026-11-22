#include "composition_target.hpp"
#include "directx_error.hpp"
#include <Windows.h>
#include <dcomp.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#pragma comment(lib, "dcomp.lib")
namespace mousebox::render::detail
{
struct CompositionTarget::Impl
{
    Microsoft::WRL::ComPtr<IDCompositionDevice> device;
    Microsoft::WRL::ComPtr<IDCompositionTarget> target;
    Microsoft::WRL::ComPtr<IDCompositionVisual> visual;
    Impl(HWND window, IDXGIDevice *dxgi, IDXGISwapChain1 *chain)
    {
        check(DCompositionCreateDevice(dxgi, IID_PPV_ARGS(&device)), "DirectComposition device");
        check(device->CreateTargetForHwnd(window, TRUE, &target), "DirectComposition target");
        check(device->CreateVisual(&visual), "DirectComposition visual");
        check(visual->SetContent(chain), "DirectComposition swap chain");
        check(target->SetRoot(visual.Get()), "DirectComposition root");
        check(device->Commit(), "DirectComposition commit");
    }
};
CompositionTarget::CompositionTarget(void *window, IDXGIDevice *dxgi, IDXGISwapChain1 *chain)
    : impl_(std::make_unique<Impl>(static_cast<HWND>(window), dxgi, chain))
{
}
CompositionTarget::~CompositionTarget() = default;
} // namespace mousebox::render::detail
