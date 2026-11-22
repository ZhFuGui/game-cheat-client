#pragma once
#include <cstdint>
#include <functional>
#include <memory>

struct ID3D11Device;
struct ID3D11DeviceContext;
namespace mousebox::render
{
// Optional telemetry only. A GPU fence event wakes the existing render worker;
// never waits for GPU completion, copies pixels, or adds a repaint timer.
class GpuCompletion
{
    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    GpuCompletion(ID3D11Device *, ID3D11DeviceContext *);
    ~GpuCompletion();
    void *event() const;
    bool available() const;
    std::uint64_t signal();
    void presented(std::uint64_t, std::function<void(double localQpcMs)>);
    void collect();
};
} // namespace mousebox::render
