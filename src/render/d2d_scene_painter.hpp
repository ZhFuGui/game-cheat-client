#pragma once
#include "primitives.hpp"
#include <cstdint>
#include <cstddef>
#include <memory>
#include <span>
#include <optional>

struct ID2D1Factory1;
struct ID2D1DeviceContext;
struct ID2D1Bitmap1;
namespace mousebox::render::detail
{
// Render-thread-owned D2D paint resources and text/geometry caches. This
// component has no swap-chain, mailbox, timing loop or game dependency.
class D2DScenePainter
{
    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    D2DScenePainter(ID2D1Factory1 *, ID2D1DeviceContext *, std::span<const std::byte> font);
    ~D2DScenePainter();
    void resize(unsigned widthPixels, unsigned heightPixels, float dpiScale);
    void draw(const Primitives &, const Palette &, bool english, std::optional<std::uint64_t> drawFramesPerSecond,
              ID2D1Bitmap1 *meshOverlay = nullptr, bool transparent = false);
};
} // namespace mousebox::render::detail
