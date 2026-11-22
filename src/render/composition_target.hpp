#pragma once
#include <memory>
struct IDXGIDevice;
struct IDXGISwapChain1;
namespace mousebox::render::detail
{
class CompositionTarget
{
    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    CompositionTarget(void *window, IDXGIDevice *, IDXGISwapChain1 *);
    ~CompositionTarget();
};
} // namespace mousebox::render::detail
