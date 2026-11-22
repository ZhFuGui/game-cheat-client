#include "gpu_completion.hpp"
#include "frame_clock.hpp"
#include <Windows.h>
#include <d3d11_4.h>
#include <wrl/client.h>
#include <deque>

namespace mousebox::render
{
using Microsoft::WRL::ComPtr;
struct GpuCompletion::Impl
{
    struct Pending
    {
        std::uint64_t value;
        std::function<void(double)> completed;
    };
    HANDLE completed = nullptr;
    ComPtr<ID3D11Fence> fence;
    ComPtr<ID3D11DeviceContext4> context;
    std::deque<Pending> pending;
    std::uint64_t next = 0, armed = 0;
    bool enabled = false;
    ~Impl()
    {
        // Drop device fence registrations before closing their event.
        fence.Reset();
        if (completed)
            CloseHandle(completed);
    }
    void arm()
    {
        if (pending.empty() || armed == pending.front().value || !enabled)
            return;
        armed = pending.front().value;
        if (FAILED(fence->SetEventOnCompletion(armed, completed)))
        {
            enabled = false;
            pending.clear();
        }
    }
};
GpuCompletion::GpuCompletion(ID3D11Device *device, ID3D11DeviceContext *context) : impl_(std::make_unique<Impl>())
{
    auto &d = *impl_;
    d.completed = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    ComPtr<ID3D11Device5> latest;
    d.enabled = d.completed && frameClockMilliseconds().has_value() &&
                SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&latest))) &&
                SUCCEEDED(context->QueryInterface(IID_PPV_ARGS(&d.context))) &&
                SUCCEEDED(latest->CreateFence(0, D3D11_FENCE_FLAG_NONE, IID_PPV_ARGS(&d.fence)));
}
GpuCompletion::~GpuCompletion() = default;
void *GpuCompletion::event() const
{
    return impl_->completed;
}
bool GpuCompletion::available() const
{
    return impl_->enabled;
}
std::uint64_t GpuCompletion::signal()
{
    auto &d = *impl_;
    // DXGI already bounds render work. If telemetry is delayed nevertheless,
    // drop measurement tickets rather than blocking drawing or growing memory.
    if (!d.enabled || d.pending.size() >= 8)
        return 0;
    const auto value = ++d.next;
    if (FAILED(d.context->Signal(d.fence.Get(), value)))
    {
        d.enabled = false;
        d.pending.clear();
        return 0;
    }
    return value;
}
void GpuCompletion::presented(std::uint64_t value, std::function<void(double)> completed)
{
    if (!value || !impl_->enabled)
        return;
    impl_->pending.push_back({value, std::move(completed)});
    impl_->arm();
}
void GpuCompletion::collect()
{
    auto &d = *impl_;
    if (!d.enabled || d.pending.empty())
        return;
    const auto completed = d.fence->GetCompletedValue();
    if (completed == UINT64_MAX)
    {
        d.enabled = false;
        d.pending.clear();
        return;
    }
    const auto observedAtMs = frameClockMilliseconds();
    if (!observedAtMs)
        return;
    while (!d.pending.empty() && d.pending.front().value <= completed)
    {
        auto ticket = std::move(d.pending.front());
        d.pending.pop_front();
        d.armed = 0;
        // CPU observation of GPU completion includes worker wakeup scheduling.
        // It is not monitor scanout / photon time.
        if (ticket.completed)
            ticket.completed(*observedAtMs);
    }
    d.arm();
}
} // namespace mousebox::render
