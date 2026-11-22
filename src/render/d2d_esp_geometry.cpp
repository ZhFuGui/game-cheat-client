#include "d2d_esp_geometry.hpp"
#include "directx_error.hpp"
#include <algorithm>
#include <cmath>

namespace mousebox::render::detail
{
namespace
{
void circle(ID2D1GeometrySink *sink, Point p, float r, bool clockwise = true)
{
    if (!std::isfinite(r) || r <= 0)
        return;
    const auto sweep = clockwise ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;
    sink->BeginFigure({p.x - r, p.y}, D2D1_FIGURE_BEGIN_FILLED);
    sink->AddArc(D2D1::ArcSegment({p.x + r, p.y}, {r, r}, 0, sweep, D2D1_ARC_SIZE_SMALL));
    sink->AddArc(D2D1::ArcSegment({p.x - r, p.y}, {r, r}, 0, sweep, D2D1_ARC_SIZE_SMALL));
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
}
void stroke(ID2D1GeometrySink *sink, const ScaledLine &s, float multiplier)
{
    const float dx = s.b.x - s.a.x, dy = s.b.y - s.a.y, length = std::hypot(dx, dy), a = s.widthA * multiplier / 2,
                b = s.widthB * multiplier / 2;
    if (!std::isfinite(a) || !std::isfinite(b) || a <= 0 || b <= 0 || !std::isfinite(length))
        return;
    if (length > 0)
    {
        const float nx = -dy / length, ny = dx / length;
        sink->BeginFigure({s.a.x + nx * a, s.a.y + ny * a}, D2D1_FIGURE_BEGIN_FILLED);
        sink->AddLine({s.a.x - nx * a, s.a.y - ny * a});
        sink->AddLine({s.b.x - nx * b, s.b.y - ny * b});
        sink->AddLine({s.b.x + nx * b, s.b.y + ny * b});
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    }
    circle(sink, s.a, a);
    circle(sink, s.b, b);
}
} // namespace
Microsoft::WRL::ComPtr<ID2D1PathGeometry> espGeometry(ID2D1Factory1 *factory, std::span<const ScaledLine> lines,
                                                      std::span<const Circle> heads, std::span<const Circle> dots,
                                                      float multiplier)
{
    Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
    check(factory->CreatePathGeometry(&path), "Scaled ESP geometry");
    Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
    check(path->Open(&sink), "Scaled ESP sink");
    sink->SetFillMode(D2D1_FILL_MODE_WINDING);
    for (const auto &line : lines)
        stroke(sink.Get(), line, multiplier);
    for (const auto &head : heads)
    {
        const float half = head.strokeWidth * multiplier / 2;
        if (head.radius > 0 && half > 0)
        {
            circle(sink.Get(), head.center, head.radius + half);
            circle(sink.Get(), head.center, head.radius - half, false);
        }
    }
    for (const auto &dot : dots)
        circle(sink.Get(), dot.center, dot.radius);
    check(sink->Close(), "Scaled ESP close");
    return path;
}
} // namespace mousebox::render::detail
