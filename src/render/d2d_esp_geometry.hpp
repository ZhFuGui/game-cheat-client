#pragma once
#include "primitives.hpp"
#include <d2d1_1.h>
#include <wrl/client.h>
#include <span>

namespace mousebox::render::detail
{
// Filled, tapered strokes batch arbitrary per-endpoint widths without pixel
// buckets, a fixed-width outline, or one draw call for every bone.
Microsoft::WRL::ComPtr<ID2D1PathGeometry> espGeometry(ID2D1Factory1 *, std::span<const ScaledLine>,
                                                      std::span<const Circle> heads, std::span<const Circle> dots,
                                                      float strokeMultiplier = 1);
} // namespace mousebox::render::detail
