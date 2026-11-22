#include "d3d_mesh_painter.hpp"
#include "directx_error.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d2d1_1.h>
#include <wrl/client.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#pragma comment(lib, "d3dcompiler.lib")

namespace mousebox::render::detail
{
using Microsoft::WRL::ComPtr;
namespace
{
constexpr char Shader[] = R"(
struct Vertex { float4 position : POSITION; float4 color : COLOR; };
struct Fragment { float4 position : SV_POSITION; nointerpolation float4 color : COLOR; };
Fragment vertexMain(Vertex v) { Fragment f; f.position=v.position; f.color=v.color; return f; }
float4 pixelMain(Fragment f) : SV_TARGET { return f.color; }
)";
ComPtr<ID3DBlob> compile(const char *entry, const char *target)
{
    ComPtr<ID3DBlob> code, errors;
    const auto result = D3DCompile(Shader, sizeof(Shader) - 1, nullptr, nullptr, nullptr, entry, target,
                                   D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &errors);
    check(result, "Mesh shader compilation");
    return code;
}
} // namespace
struct D3DMeshPainter::Impl
{
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID2D1DeviceContext> d2d;
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
    ComPtr<ID3D11InputLayout> layout;
    ComPtr<ID3D11Buffer> vertices;
    ComPtr<ID3D11DepthStencilState> depthState;
    ComPtr<ID3D11RasterizerState> solid, wire;
    ComPtr<ID3D11Texture2D> color, depth;
    ComPtr<ID3D11RenderTargetView> target;
    ComPtr<ID3D11DepthStencilView> depthView;
    ComPtr<ID2D1Bitmap1> bitmap;
    std::size_t capacity = 0;
    unsigned width = 0, height = 0;
    float dpi = 0;
    Impl(ID3D11Device *dev, ID3D11DeviceContext *ctx, ID2D1DeviceContext *dc) : device(dev), context(ctx), d2d(dc)
    {
        const auto vs = compile("vertexMain", "vs_5_0"), ps = compile("pixelMain", "ps_5_0");
        check(device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &vertexShader),
              "Mesh vertex shader");
        check(device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &pixelShader),
              "Mesh pixel shader");
        constexpr D3D11_INPUT_ELEMENT_DESC elements[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0}};
        check(device->CreateInputLayout(elements, 2, vs->GetBufferPointer(), vs->GetBufferSize(), &layout),
              "Mesh layout");
        D3D11_DEPTH_STENCIL_DESC depthDesc{};
        depthDesc.DepthEnable = TRUE;
        depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
        check(device->CreateDepthStencilState(&depthDesc, &depthState), "Mesh depth state");
        D3D11_RASTERIZER_DESC raster{};
        raster.CullMode = D3D11_CULL_NONE;
        raster.DepthClipEnable = TRUE;
        raster.FillMode = D3D11_FILL_SOLID;
        check(device->CreateRasterizerState(&raster, &solid), "Mesh solid state");
        raster.FillMode = D3D11_FILL_WIREFRAME;
        check(device->CreateRasterizerState(&raster, &wire), "Mesh wire state");
    }
    void resize(unsigned w, unsigned h, float scale)
    {
        if (width == w && height == h && dpi == scale)
            return;
        bitmap.Reset();
        target.Reset();
        depthView.Reset();
        color.Reset();
        depth.Reset();
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = w;
        desc.Height = h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        check(device->CreateTexture2D(&desc, nullptr, &color), "Mesh color target");
        check(device->CreateRenderTargetView(color.Get(), nullptr, &target), "Mesh target view");
        desc.Format = DXGI_FORMAT_D32_FLOAT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        check(device->CreateTexture2D(&desc, nullptr, &depth), "Mesh depth target");
        check(device->CreateDepthStencilView(depth.Get(), nullptr, &depthView), "Mesh depth view");
        ComPtr<IDXGISurface> surface;
        check(color.As(&surface), "Mesh DXGI surface");
        const auto properties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_NONE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96 * scale, 96 * scale);
        check(d2d->CreateBitmapFromDxgiSurface(surface.Get(), &properties, &bitmap), "Mesh composition bitmap");
        width = w;
        height = h;
        dpi = scale;
    }
    ID2D1Bitmap1 *draw(const TriangleMesh &mesh, unsigned w, unsigned h, float scale)
    {
        if (mesh.vertices.empty() || !w || !h || mesh.viewport.width <= 0 || mesh.viewport.height <= 0)
            return nullptr;
        if (mesh.vertices.size() > 2000000)
            throw std::invalid_argument("Mesh vertex budget exceeded");
        resize(w, h, scale);
        if (capacity < mesh.vertices.size())
        {
            vertices.Reset();
            capacity = std::min<std::size_t>(2000000, std::max<std::size_t>(4096, mesh.vertices.size() * 2));
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = static_cast<UINT>(capacity * sizeof(ClipVertex));
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            check(device->CreateBuffer(&desc, nullptr, &vertices), "Mesh vertex buffer");
        }
        D3D11_MAPPED_SUBRESOURCE mapped{};
        check(context->Map(vertices.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped), "Mesh upload");
        std::memcpy(mapped.pData, mesh.vertices.data(), mesh.vertices.size() * sizeof(ClipVertex));
        context->Unmap(vertices.Get(), 0);
        constexpr float clear[4]{};
        context->ClearRenderTargetView(target.Get(), clear);
        context->ClearDepthStencilView(depthView.Get(), D3D11_CLEAR_DEPTH, 0, 0);
        const auto renderTarget = target.Get();
        context->OMSetRenderTargets(1, &renderTarget, depthView.Get());
        context->OMSetDepthStencilState(depthState.Get(), 0);
        context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
        const D3D11_VIEWPORT viewport{mesh.viewport.x * scale,
                                      mesh.viewport.y * scale,
                                      mesh.viewport.width * scale,
                                      mesh.viewport.height * scale,
                                      0,
                                      1};
        context->RSSetViewports(1, &viewport);
        context->RSSetState(mesh.wireframe ? wire.Get() : solid.Get());
        const auto buffer = vertices.Get();
        constexpr UINT stride = sizeof(ClipVertex), offset = 0;
        context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
        context->IASetInputLayout(layout.Get());
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(vertexShader.Get(), nullptr, 0);
        context->PSSetShader(pixelShader.Get(), nullptr, 0);
        context->GSSetShader(nullptr, nullptr, 0);
        context->HSSetShader(nullptr, nullptr, 0);
        context->DSSetShader(nullptr, nullptr, 0);
        context->Draw(static_cast<UINT>(mesh.vertices.size()), 0);
        context->OMSetRenderTargets(0, nullptr, nullptr);
        return bitmap.Get();
    }
};
D3DMeshPainter::D3DMeshPainter(ID3D11Device *d, ID3D11DeviceContext *c, ID2D1DeviceContext *dc)
    : impl_(std::make_unique<Impl>(d, c, dc))
{
}
D3DMeshPainter::~D3DMeshPainter() = default;
ID2D1Bitmap1 *D3DMeshPainter::draw(const TriangleMesh &m, unsigned w, unsigned h, float s)
{
    return impl_->draw(m, w, h, s);
}
} // namespace mousebox::render::detail
