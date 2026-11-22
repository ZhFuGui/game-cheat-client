#pragma once
#include "scene_submission.hpp"
#include "graphics_adapter.hpp"
#include "frame_timing.hpp"
#include <cstddef>
#include <memory>
#include <span>
#include <string>

namespace mousebox::render
{
enum class SurfaceAlpha
{
    Opaque,
    Premultiplied
};
enum class AdapterFallback
{
    None,
    RequestedMissing,
    RequestedUnavailable,
    HardwareUnavailable
};
struct RenderStats
{
    bool initialized = false, hardware = false;
    bool gpuCompletionAvailable = false;
    std::uint64_t drawFramesPerSecond = 0;
    std::wstring adapter, error;
    std::string requestedAdapter = "auto", adapterId;
    AdapterFallback adapterFallback = AdapterFallback::None;
    std::uint64_t submitted = 0, drawn = 0, presented = 0, coalesced = 0, lastRevision = 0, deviceRecoveries = 0;
    double projectionMs = 0, drawMs = 0;
    EspSummary esp;
    FrameTimingSnapshot timing;
};
struct Capture
{
    unsigned width = 0, height = 0;
    std::vector<std::uint8_t> bgra;
};
// Only immutable scene pointers cross the mailbox. D3D/D2D resources belong to
// one worker; Reader/UI never wait for Present or perform GPU calls.
class DirectXRenderer
{
    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    // Font bytes are borrowed until the renderer is destroyed. They can be
    // read again when the worker recreates a lost device.
    DirectXRenderer(void *nativeWindow, std::span<const std::byte> font, bool forceSoftwareForTest = false,
                    std::string preferredAdapter = "auto", SurfaceAlpha = SurfaceAlpha::Opaque);
    ~DirectXRenderer();
    DirectXRenderer(const DirectXRenderer &) = delete;
    DirectXRenderer &operator=(const DirectXRenderer &) = delete;
    void submit(SceneSubmission, unsigned widthPixels, unsigned heightPixels, float dpiScale, bool visible);
    RenderStats stats() const;
    void requestCapture(); // Diagnostic readback only; absent from the live render path.
    Capture capture() const;
    void recreateDevice();
    void setAdapter(std::string id);
};
} // namespace mousebox::render
