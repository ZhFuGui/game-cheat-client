#include "directx_renderer.hpp"
#include "gpu_completion.hpp"
#include "d2d_scene_painter.hpp"
#include "d3d_mesh_painter.hpp"
#include "composition_target.hpp"
#include "dxgi_adapters.hpp"
#include "directx_error.hpp"
#include "domain/rolling_counter.hpp"
#include "frame_clock.hpp"
#include <Windows.h>
#include <d3d11.h>
#include <d2d1_1.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace mousebox::render
{
using detail::check;
using detail::NativeAdapter;
using detail::nativeAdapters;
using Microsoft::WRL::ComPtr;
namespace
{
struct Handle
{
    HANDLE value = nullptr;
    ~Handle()
    {
        if (value)
            CloseHandle(value);
    }
    void reset(HANDLE next = nullptr)
    {
        if (value)
            CloseHandle(value);
        value = next;
    }
};
double elapsed(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}
std::uint64_t rateTime()
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

} // namespace

struct DirectXRenderer::Impl
{
    struct Request
    {
        SceneSubmission scene;
        unsigned width = 0, height = 0;
        float scale = 1;
        bool visible = false;
        std::uint64_t serial = 0;
        std::string adapter = "auto";
    };
    HWND window;
    bool forceSoftware;
    bool transparent;
    mutable std::mutex mutex;
    Request latest;
    RenderStats statistics;
    RollingCounter drawFrames;
    FrameTiming timing;
    Capture captured;
    bool captureRequested = false, recreateRequested = false;
    Handle changed, shutdown, latency;
    std::jthread worker;
    ComPtr<ID3D11Device> d3d;
    ComPtr<ID3D11DeviceContext> d3dContext;
    std::span<const std::byte> font;
    std::unique_ptr<GpuCompletion> completion;
    ComPtr<ID2D1Factory1> factory;
    ComPtr<ID2D1Device> d2d;
    ComPtr<ID2D1DeviceContext> context;
    ComPtr<IDXGISwapChain2> swap;
    std::unique_ptr<detail::CompositionTarget> composition;
    ComPtr<ID2D1Bitmap1> target;
    std::unique_ptr<detail::D2DScenePainter> painter;
    std::unique_ptr<detail::D3DMeshPainter> meshPainter;
    unsigned width = 0, height = 0;
    float scale = 0;
    std::string activePreference;
    std::unique_ptr<ProjectionState> projector;
    Impl(HWND hwnd, std::span<const std::byte> fontBytes, bool software, std::string preference, SurfaceAlpha alpha)
        : window(hwnd), forceSoftware(software), transparent(alpha == SurfaceAlpha::Premultiplied), font(fontBytes)
    {
        latest.adapter = preference.empty() ? "auto" : std::move(preference);
        statistics.requestedAdapter = latest.adapter;
        changed.value = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        shutdown.value = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!changed.value || !shutdown.value)
            throw std::runtime_error("Cannot create renderer events");
        worker = std::jthread([this] { run(); });
    }
    ~Impl()
    {
        SetEvent(shutdown.value);
        if (worker.joinable())
            worker.join();
    }
    void release()
    {
        completion.reset();
        if (context)
            context->SetTarget(nullptr);
        painter.reset();
        meshPainter.reset();
        target.Reset();
        context.Reset();
        d2d.Reset();
        factory.Reset();
        composition.reset();
        swap.Reset();
        latency.reset();
        d3dContext.Reset();
        d3d.Reset();
        width = height = 0;
        scale = 0;
    }
    void initializeOnAdapter(const Request &request, const NativeAdapter *selected, AdapterFallback fallback)
    {
        const D3D_FEATURE_LEVEL levels[]{D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
        check(D3D11CreateDevice(selected ? selected->adapter.Get() : nullptr,
                                selected ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_WARP, nullptr,
                                D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, 3, D3D11_SDK_VERSION, &d3d, nullptr,
                                &d3dContext),
              "D3D11CreateDevice");
        ComPtr<IDXGIDevice1> dxgi;
        check(d3d.As(&dxgi), "DXGI device");
        check(dxgi->SetMaximumFrameLatency(1), "Frame latency");
        ComPtr<IDXGIAdapter> adapter;
        check(dxgi->GetAdapter(&adapter), "DXGI adapter");
        DXGI_ADAPTER_DESC adapterDescription{};
        check(adapter->GetDesc(&adapterDescription), "Adapter description");
        ComPtr<IDXGIFactory2> dxgiFactory;
        check(adapter->GetParent(IID_PPV_ARGS(&dxgiFactory)), "DXGI factory");
        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width = request.width;
        desc.Height = request.height;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.AlphaMode = transparent ? DXGI_ALPHA_MODE_PREMULTIPLIED : DXGI_ALPHA_MODE_IGNORE;
        desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        ComPtr<IDXGISwapChain1> chain;
        if (transparent)
        {
            check(dxgiFactory->CreateSwapChainForComposition(d3d.Get(), &desc, nullptr, &chain),
                  "Composition swap chain");
            composition = std::make_unique<detail::CompositionTarget>(window, dxgi.Get(), chain.Get());
        }
        else
            check(dxgiFactory->CreateSwapChainForHwnd(d3d.Get(), window, &desc, nullptr, nullptr, &chain),
                  "CreateSwapChainForHwnd");
        check(chain.As(&swap), "Swap chain 2");
        check(swap->SetMaximumFrameLatency(1), "Swap chain latency");
        latency.reset(swap->GetFrameLatencyWaitableObject());
        if (!latency.value)
            throw std::runtime_error("No swap-chain latency event");
        check(dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER), "Window association");
        check(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&factory)), "D2D factory");
        check(factory->CreateDevice(dxgi.Get(), &d2d), "D2D device");
        check(d2d->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &context), "D2D context");
        painter = std::make_unique<detail::D2DScenePainter>(factory.Get(), context.Get(), font);
        resize(request);
        completion = std::make_unique<GpuCompletion>(d3d.Get(), d3dContext.Get());
        activePreference = request.adapter;
        const std::lock_guard lock(mutex);
        statistics.initialized = true;
        statistics.hardware = selected && !selected->info.software;
        statistics.adapter = adapterDescription.Description;
        statistics.adapterId = selected ? selected->info.id : "warp";
        statistics.adapterFallback = fallback;
        statistics.gpuCompletionAvailable = completion->available();
    }
    void initialize(const Request &request)
    {
        if (forceSoftware || request.adapter == "warp")
        {
            initializeOnAdapter(request, nullptr, AdapterFallback::None);
            return;
        }
        std::vector<NativeAdapter> adapters;
        try
        {
            adapters = nativeAdapters();
        }
        catch (const std::exception &)
        {
            initializeOnAdapter(request, nullptr, AdapterFallback::HardwareUnavailable);
            return;
        }
        const bool manual = request.adapter != "auto";
        AdapterFallback fallback = manual ? AdapterFallback::RequestedMissing : AdapterFallback::None;
        const NativeAdapter *requested = nullptr;
        for (const auto &item : adapters)
            if (item.info.id == request.adapter)
            {
                requested = &item;
                break;
            }
        if (requested)
        {
            try
            {
                initializeOnAdapter(request, requested, AdapterFallback::None);
                return;
            }
            catch (const std::exception &)
            {
                release();
                fallback = AdapterFallback::RequestedUnavailable;
            }
        }
        for (const auto &item : adapters)
            if (!item.info.software && &item != requested)
            {
                try
                {
                    initializeOnAdapter(request, &item, fallback);
                    return;
                }
                catch (const std::exception &)
                {
                    release();
                }
            }
        initializeOnAdapter(request, nullptr, manual ? fallback : AdapterFallback::HardwareUnavailable);
    }
    void resize(const Request &request)
    {
        if (target && width == request.width && height == request.height && scale == request.scale)
            return;
        context->SetTarget(nullptr);
        target.Reset();
        if (width && (width != request.width || height != request.height))
            check(swap->ResizeBuffers(2, request.width, request.height, DXGI_FORMAT_B8G8R8A8_UNORM,
                                      DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT),
                  "ResizeBuffers");
        width = request.width;
        height = request.height;
        scale = request.scale;
        ComPtr<IDXGISurface> surface;
        check(swap->GetBuffer(0, IID_PPV_ARGS(&surface)), "Swap buffer");
        const auto properties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                              transparent ? D2D1_ALPHA_MODE_PREMULTIPLIED : D2D1_ALPHA_MODE_IGNORE),
            96 * scale, 96 * scale);
        check(context->CreateBitmapFromDxgiSurface(surface.Get(), &properties, &target), "D2D swap target");
        context->SetTarget(target.Get());
        context->SetDpi(96 * scale, 96 * scale);
        painter->resize(width, height, scale);
    }
    void readback()
    {
        ComPtr<ID3D11Texture2D> texture;
        check(swap->GetBuffer(0, IID_PPV_ARGS(&texture)), "Capture buffer");
        D3D11_TEXTURE2D_DESC desc{};
        texture->GetDesc(&desc);
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;
        ComPtr<ID3D11Texture2D> staging;
        check(d3d->CreateTexture2D(&desc, nullptr, &staging), "Capture staging");
        d3dContext->CopyResource(staging.Get(), texture.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        check(d3dContext->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped), "Capture map");
        Capture next;
        next.width = width;
        next.height = height;
        next.bgra.resize(static_cast<std::size_t>(width) * height * 4);
        for (unsigned y = 0; y < height; ++y)
            std::memcpy(next.bgra.data() + static_cast<std::size_t>(y) * width * 4,
                        static_cast<const std::uint8_t *>(mapped.pData) + static_cast<std::size_t>(y) * mapped.RowPitch,
                        static_cast<std::size_t>(width) * 4);
        d3dContext->Unmap(staging.Get(), 0);
        const std::lock_guard lock(mutex);
        captured = std::move(next);
    }
    void run() noexcept
    {
        const auto com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        std::uint64_t processed = 0;
        bool dirty = false;
        for (;;)
        {
            HANDLE events[]{shutdown.value, changed.value, nullptr, nullptr};
            DWORD count = 2;
            const auto completionEvent = completion ? completion->event() : nullptr;
            if (completionEvent)
                events[count++] = completionEvent;
            const auto readyEvent = dirty ? latency.value : nullptr;
            if (readyEvent)
                events[count++] = readyEvent;
            const auto event = WaitForMultipleObjects(count, events, FALSE, INFINITE);
            if (event == WAIT_OBJECT_0 || event == WAIT_FAILED)
                break;
            const auto wake = events[event - WAIT_OBJECT_0];
            if (completion)
                completion->collect();
            if (wake == completionEvent && !dirty)
                continue;
            Request request;
            bool captureNow = false, reset = false;
            {
                const std::lock_guard lock(mutex);
                request = latest;
                reset = std::exchange(recreateRequested, false);
            }
            if (reset)
            {
                release();
                const std::lock_guard lock(mutex);
                statistics.initialized = false;
                statistics.adapter.clear();
                statistics.adapterId.clear();
                captured = {};
                ++statistics.deviceRecoveries;
            }
            if (!request.visible || !request.width || !request.height)
            {
                dirty = false;
                processed = request.serial;
                continue;
            }
            if (!dirty && processed == request.serial && !reset)
                continue;
            try
            {
                if (!d3d)
                    initialize(request);
                // Data events replace the mailbox while GPU back-pressure waits.
                // The frame-ready event, not a timer, wakes the next draw.
                if (wake != latency.value)
                {
                    const auto ready = WaitForSingleObject(latency.value, 0);
                    if (ready != WAIT_OBJECT_0)
                    {
                        dirty = true;
                        continue;
                    }
                }
                // Refresh immediately before projection to avoid drawing the old
                // request captured while the previous GPU frame was completing.
                {
                    const std::lock_guard lock(mutex);
                    request = latest;
                    captureNow = std::exchange(captureRequested, false);
                }
                if (!request.visible || !request.width || !request.height)
                {
                    dirty = false;
                    continue;
                }
                if (request.adapter != activePreference)
                {
                    dirty = true;
                    continue;
                } // A newer adapter change is already queued.
                const auto drawingStarted = frameClockMilliseconds();
                resize(request);
                const auto start = std::chrono::steady_clock::now();
                Primitives primitives;
                if (request.scene.projection)
                {
                    const auto &projection = *request.scene.projection;
                    if (!projector || !projection.accepts(*projector))
                        projector = projection.createState();
                    primitives =
                        projection.project(*projector, request.width / request.scale, request.height / request.scale);
                }
                else
                    projector.reset();
                const double projection = elapsed(start);
                const auto drawStart = std::chrono::steady_clock::now();
                std::optional<std::uint64_t> framesPerSecond;
                if (primitives.esp && completion && completion->available())
                {
                    const std::lock_guard lock(mutex);
                    framesPerSecond = drawFrames.count(rateTime());
                }
                ID2D1Bitmap1 *meshOverlay = nullptr;
                if (primitives.esp && primitives.esp->mesh && !primitives.esp->mesh->vertices.empty())
                {
                    if (!meshPainter)
                        meshPainter =
                            std::make_unique<detail::D3DMeshPainter>(d3d.Get(), d3dContext.Get(), context.Get());
                    meshOverlay =
                        meshPainter->draw(*primitives.esp->mesh, request.width, request.height, request.scale);
                }
                painter->draw(primitives, request.scene.palette, request.scene.english, framesPerSecond, meshOverlay,
                              transparent);
                if (captureNow)
                    readback();
                const auto fence = completion->signal();
                const auto hr = swap->Present(0, DXGI_PRESENT_DO_NOT_WAIT);
                if (hr != DXGI_ERROR_WAS_STILL_DRAWING)
                    check(hr, "Present");
                if (hr == S_OK && fence)
                {
                    auto frame = request.scene.receipt && drawingStarted
                                     ? std::optional(FrameDrawStart{*request.scene.receipt, *drawingStarted})
                                     : std::nullopt;
                    completion->presented(fence, [this, notify = std::move(request.scene.completed),
                                                  frame = std::move(frame)](double now) {
                        {
                            const std::lock_guard lock(mutex);
                            drawFrames.add(rateTime());
                        }
                        if (notify)
                            notify(now);
                        if (frame)
                        {
                            const std::lock_guard lock(mutex);
                            timing.completed(*frame, now);
                        }
                    });
                }
                {
                    const std::lock_guard lock(mutex);
                    ++statistics.drawn;
                    if (hr == S_OK)
                        ++statistics.presented;
                    statistics.coalesced += request.serial > processed ? request.serial - processed - 1 : 0;
                    statistics.lastRevision = request.scene.revision;
                    statistics.projectionMs = projection;
                    statistics.drawMs = elapsed(drawStart);
                    statistics.esp = primitives.esp ? primitives.esp->summary : EspSummary{};
                    statistics.error.clear();
                    statistics.gpuCompletionAvailable = completion->available();
                }
                processed = request.serial;
                dirty = hr == DXGI_ERROR_WAS_STILL_DRAWING;
            }
            catch (const std::exception &error)
            {
                release();
                processed = request.serial;
                dirty = false;
                const std::string message = error.what();
                const std::lock_guard lock(mutex);
                statistics.initialized = false;
                statistics.error.assign(message.begin(), message.end());
                ++statistics.deviceRecoveries;
            }
        }
        projector.reset();
        release();
        if (SUCCEEDED(com))
            CoUninitialize();
    }
};
DirectXRenderer::DirectXRenderer(void *window, std::span<const std::byte> font, bool software, std::string adapter,
                                 SurfaceAlpha alpha)
    : impl_(std::make_unique<Impl>(static_cast<HWND>(window), font, software, std::move(adapter), alpha))
{
}
DirectXRenderer::~DirectXRenderer() = default;
void DirectXRenderer::submit(SceneSubmission scene, unsigned width, unsigned height, float scale, bool visible)
{
    {
        const std::lock_guard lock(impl_->mutex);
        impl_->timing.select(scene.receipt);
        impl_->latest = {std::move(scene),
                         width,
                         height,
                         std::isfinite(scale) && scale > 0 ? scale : 1,
                         visible,
                         impl_->latest.serial + 1,
                         impl_->latest.adapter};
        ++impl_->statistics.submitted;
    }
    SetEvent(impl_->changed.value);
}
RenderStats DirectXRenderer::stats() const
{
    const std::lock_guard lock(impl_->mutex);
    auto value = impl_->statistics;
    value.drawFramesPerSecond = impl_->drawFrames.count(rateTime());
    value.timing = impl_->timing.snapshot(frameClockMilliseconds().value_or(-1));
    return value;
}
void DirectXRenderer::requestCapture()
{
    {
        const std::lock_guard lock(impl_->mutex);
        impl_->captureRequested = true;
        ++impl_->latest.serial;
    }
    SetEvent(impl_->changed.value);
}
Capture DirectXRenderer::capture() const
{
    const std::lock_guard lock(impl_->mutex);
    return impl_->captured;
}
void DirectXRenderer::recreateDevice()
{
    {
        const std::lock_guard lock(impl_->mutex);
        impl_->recreateRequested = true;
        ++impl_->latest.serial;
    }
    SetEvent(impl_->changed.value);
}
void DirectXRenderer::setAdapter(std::string id)
{
    if (id.empty())
        id = "auto";
    {
        const std::lock_guard lock(impl_->mutex);
        if (impl_->latest.adapter == id)
            return;
        impl_->latest.adapter = id;
        impl_->statistics.requestedAdapter = std::move(id);
        impl_->recreateRequested = true;
        ++impl_->latest.serial;
    }
    SetEvent(impl_->changed.value);
}
} // namespace mousebox::render
