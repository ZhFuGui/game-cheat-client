#pragma once
#include "triangle_mesh.hpp"
#include <memory>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID2D1DeviceContext;
struct ID2D1Bitmap1;
namespace mousebox::render::detail
{
// Same render-thread lifetime as the existing D2D painter. GPU-only composition,
// no pixel readback and no game, source, session or visibility dependencies.
class D3DMeshPainter
{
    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    D3DMeshPainter(ID3D11Device *, ID3D11DeviceContext *, ID2D1DeviceContext *);
    ~D3DMeshPainter();
    ID2D1Bitmap1 *draw(const TriangleMesh &, unsigned widthPixels, unsigned heightPixels, float dpiScale);
};
} // namespace mousebox::render::detail
