#include "d2d_scene_painter.hpp"
#include "directx_error.hpp"
#include "d2d_esp_geometry.hpp"
#include "bundled_font.hpp"
#include <d2d1_1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace mousebox::render::detail
{
using Microsoft::WRL::ComPtr;
namespace
{
D2D1_POINT_2F point(Point p)
{
    return D2D1::Point2F(p.x, p.y);
}
void circle(ID2D1GeometrySink *sink, Point p, float radius)
{
    sink->BeginFigure(D2D1::Point2F(p.x - radius, p.y), D2D1_FIGURE_BEGIN_FILLED);
    sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(p.x + radius, p.y), D2D1::SizeF(radius, radius), 0,
                                  D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
    sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(p.x - radius, p.y), D2D1::SizeF(radius, radius), 0,
                                  D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
}
} // namespace
struct D2DScenePainter::Impl
{
    ComPtr<ID2D1Factory1> factory;
    ComPtr<ID2D1DeviceContext> context;
    ComPtr<ID2D1SolidColorBrush> entityBrush, targetBrush, gridBrush, textBrush, outlineBrush, hudBrush;
    ID2D1Bitmap1 *meshOverlay = nullptr; // Borrowed from the render-thread mesh painter for this draw only.
    std::array<ComPtr<ID2D1SolidColorBrush>, EspGroupCount> entityBrushes;
    ComPtr<IDWriteFactory> writeFactory;
    std::unique_ptr<BundledFont> primaryFont;
    ComPtr<IDWriteTextFormat> textFormat;
    struct Label
    {
        std::wstring content;
        float width = 0;
        ComPtr<IDWriteTextLayout> layout;
        DWRITE_TEXT_METRICS metrics{};
    };
    std::unordered_map<std::uint64_t, Label> labelLayouts;
    ComPtr<ID2D1PathGeometry> gridGeometry;
    unsigned width = 0, height = 0;
    float scale = 1;
    bool english = false;
    bool transparent = false;
    std::optional<std::uint64_t> drawFramesPerSecond;
    Impl(ID2D1Factory1 *sourceFactory, ID2D1DeviceContext *sourceContext, std::span<const std::byte> font)
        : factory(sourceFactory), context(sourceContext)
    {
        check(context->CreateSolidColorBrush(D2D1::ColorF(0x50c9bd), &entityBrush), "Teal brush");
        check(context->CreateSolidColorBrush(D2D1::ColorF(0xefbb68), &targetBrush), "Gold brush");
        check(context->CreateSolidColorBrush(D2D1::ColorF(0x1b2a38), &gridBrush), "Grid brush");
        check(context->CreateSolidColorBrush(D2D1::ColorF(0x8499a9), &textBrush), "Text brush");
        for (unsigned i = 0; i < entityBrushes.size(); ++i)
            check(context->CreateSolidColorBrush(D2D1::ColorF(i < EspColors.size() ? EspColors[i] : 0),
                                                 &entityBrushes[i]),
                  "ESP person brush");
        check(context->CreateSolidColorBrush(D2D1::ColorF(0x080808), &outlineBrush), "ESP outline brush");
        check(context->CreateSolidColorBrush(D2D1::ColorF(0x080808), &hudBrush), "ESP HUD brush");
        check(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                  reinterpret_cast<IUnknown **>(writeFactory.GetAddressOf())),
              "DirectWrite factory");
        primaryFont = std::make_unique<BundledFont>(writeFactory.Get(), font);
        check(writeFactory->CreateTextFormat(primaryFont->family().c_str(), primaryFont->collection(),
                                             DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                             DWRITE_FONT_STRETCH_NORMAL, 13, L"zh-cn", &textFormat),
              "Text format");
    }
    void resize(unsigned widthPixels, unsigned heightPixels, float dpiScale)
    {
        width = widthPixels;
        height = heightPixels;
        scale = dpiScale;
        check(factory->CreatePathGeometry(&gridGeometry), "Grid geometry");
        ComPtr<ID2D1GeometrySink> sink;
        check(gridGeometry->Open(&sink), "Grid sink");
        const float w = width / scale, h = height / scale;
        for (float x = 0; x < w; x += 40)
        {
            sink->BeginFigure(D2D1::Point2F(x, 0), D2D1_FIGURE_BEGIN_HOLLOW);
            sink->AddLine(D2D1::Point2F(x, h));
            sink->EndFigure(D2D1_FIGURE_END_OPEN);
        }
        for (float y = 0; y < h; y += 40)
        {
            sink->BeginFigure(D2D1::Point2F(0, y), D2D1_FIGURE_BEGIN_HOLLOW);
            sink->AddLine(D2D1::Point2F(w, y));
            sink->EndFigure(D2D1_FIGURE_END_OPEN);
        }
        check(sink->Close(), "Grid close");
    }
    template <typename AppendCircles>
    ComPtr<ID2D1PathGeometry> geometryBatch(const std::vector<Line> &lines, AppendCircles appendCircles)
    {
        ComPtr<ID2D1PathGeometry> path;
        check(factory->CreatePathGeometry(&path), "Batch geometry");
        ComPtr<ID2D1GeometrySink> sink;
        check(path->Open(&sink), "Batch sink");
        for (const auto &line : lines)
        {
            sink->BeginFigure(point(line.a), D2D1_FIGURE_BEGIN_HOLLOW);
            sink->AddLine(point(line.b));
            sink->EndFigure(D2D1_FIGURE_END_OPEN);
        }
        appendCircles(sink.Get());
        check(sink->Close(), "Batch close");
        return path;
    }
    ComPtr<ID2D1PathGeometry> geometry(const std::vector<Line> &lines, const std::vector<Point> &points, float radius)
    {
        return geometryBatch(lines, [&](ID2D1GeometrySink *sink) {
            for (auto p : points)
                circle(sink, p, radius);
        });
    }
    void draw(const Primitives &scene, const Palette &palette)
    {
        if (scene.esp)
        {
            drawEsp(*scene.esp, palette);
            return;
        }
        if (transparent)
        {
            context->BeginDraw();
            context->Clear(D2D1::ColorF(0, 0, 0, 0));
            check(context->EndDraw(), "Clear empty overlay");
            return;
        }
        entityBrush->SetColor(D2D1::ColorF(palette.entity));
        targetBrush->SetColor(D2D1::ColorF(palette.selected));
        textBrush->SetColor(D2D1::ColorF(palette.text));
        gridBrush->SetColor(D2D1::ColorF(palette.grid));
        context->BeginDraw();
        context->Clear(D2D1::ColorF(palette.background));
        context->DrawGeometry(gridGeometry.Get(), gridBrush.Get(), 1);
        const auto center = D2D1::Point2F(width / scale / 2, height / scale / 2);
        gridBrush->SetColor(D2D1::ColorF(palette.guide));
        context->DrawEllipse(D2D1::Ellipse(center, 70, 70), gridBrush.Get(), 1);
        context->DrawLine(D2D1::Point2F(center.x - 16, center.y), D2D1::Point2F(center.x + 16, center.y),
                          gridBrush.Get());
        context->DrawLine(D2D1::Point2F(center.x, center.y - 16), D2D1::Point2F(center.x, center.y + 16),
                          gridBrush.Get());
        gridBrush->SetColor(D2D1::ColorF(palette.grid));
        for (unsigned group = 0; group < 2; ++group)
        {
            auto brush = group ? targetBrush.Get() : entityBrush.Get();
            if (!scene.skeletons[group].empty() || !scene.heads[group].empty())
            {
                const auto path = geometry(scene.skeletons[group], scene.heads[group], 6);
                context->DrawGeometry(path.Get(), brush, 1.5f);
            }
            if (!scene.dots[group].empty())
            {
                const auto path = geometry({}, scene.dots[group], 2.5f);
                context->FillGeometry(path.Get(), brush);
            }
        }
        if (scene.aim)
            context->DrawEllipse(D2D1::Ellipse(point(*scene.aim), 8, 8), targetBrush.Get(), 2);
        context->DrawText(scene.title.c_str(), static_cast<UINT32>(scene.title.size()), textFormat.Get(),
                          D2D1::RectF(18, 15, width / scale - 18, 40), textBrush.Get());
        if (!scene.message.empty())
            context->DrawText(scene.message.c_str(), static_cast<UINT32>(scene.message.size()), textFormat.Get(),
                              D2D1::RectF(25, height / scale - 70, width / scale - 25, height / scale - 10),
                              textBrush.Get());
        check(context->EndDraw(), "EndDraw");
    }
    void drawEsp(const EspPrimitives &s, const Palette &palette)
    {
        entityBrushes[EspPreselectedGroup]->SetColor(D2D1::ColorF(palette.entity));
        entityBrushes[EspLockedGroup]->SetColor(D2D1::ColorF(palette.selected));
        context->BeginDraw();
        context->Clear(transparent ? D2D1::ColorF(0, 0, 0, 0) : D2D1::ColorF(palette.background));
        const auto &v = s.viewport;
        gridBrush->SetColor(D2D1::ColorF(palette.grid));
        if (!transparent)
            context->DrawGeometry(gridGeometry.Get(), gridBrush.Get(), 1);
        const auto clip = D2D1::RectF(v.x, v.y, v.x + v.width, v.y + v.height);
        gridBrush->SetColor(D2D1::ColorF(palette.guide));
        if (!transparent)
            context->DrawRectangle(clip, gridBrush.Get(), 1);
        context->PushAxisAlignedClip(clip, D2D1_ANTIALIAS_MODE_ALIASED);
        if (meshOverlay)
            context->DrawBitmap(meshOverlay, D2D1::RectF(0, 0, width / scale, height / scale), .4f,
                                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        if (s.aimRangeRadius)
            context->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(v.x + v.width / 2, v.y + v.height / 2), *s.aimRangeRadius,
                                               *s.aimRangeRadius),
                                 gridBrush.Get(), 1);
        const auto line = [&](Point a, Point b, ID2D1Brush *brush, float width = 1.f) {
            context->DrawLine(point(a), point(b), outlineBrush.Get(), width + 2);
            context->DrawLine(point(a), point(b), brush, width);
        };
        if (s.options.crosshair && v.width > 0)
        {
            const float x = v.x + v.width / 2, y = v.y + v.height / 2;
            gridBrush->SetColor(D2D1::ColorF(0xb2b2b2));
            line({x - 7, y}, {x - 2, y}, gridBrush.Get());
            line({x + 2, y}, {x + 7, y}, gridBrush.Get());
            line({x, y - 7}, {x, y - 2}, gridBrush.Get());
            line({x, y + 2}, {x, y + 7}, gridBrush.Get());
        }
        for (unsigned i = 0; i < entityBrushes.size(); ++i)
        {
            auto brush = entityBrushes[i].Get();
            if (!s.skeletons[i].empty() || !s.heads[i].empty() || !s.dots[i].empty())
            {
                const auto outline = espGeometry(factory.Get(), s.skeletons[i], s.heads[i], s.dots[i], 7.f / 3);
                const auto fill = espGeometry(factory.Get(), s.skeletons[i], s.heads[i], s.dots[i]);
                context->FillGeometry(outline.Get(), outlineBrush.Get());
                context->FillGeometry(fill.Get(), brush);
            }
        }
        for (const auto &batch : s.decorations)
        {
            if (batch.skeletons.empty() && batch.heads.empty() && batch.dots.empty())
                continue;
            gridBrush->SetColor(D2D1::ColorF(batch.color));
            const auto outline = espGeometry(factory.Get(), batch.skeletons, batch.heads, batch.dots, 7.f / 3);
            const auto fill = espGeometry(factory.Get(), batch.skeletons, batch.heads, batch.dots);
            context->FillGeometry(outline.Get(), outlineBrush.Get());
            context->FillGeometry(fill.Get(), gridBrush.Get());
        }
        std::array<std::vector<ScaledLine>, EspGroupCount> markers;
        std::array<std::vector<Circle>, EspGroupCount> points;
        for (const auto &marker : s.markers)
        {
            auto &lines = markers[marker.group];
            const auto b = marker.bounds;
            const auto annotation = marker.annotationScale;
            const auto appendLine = [&](Point a, Point b) { lines.push_back({a, b, annotation, annotation}); };
            if (s.options.marker == EspMarker::Point)
                points[marker.group].push_back({marker.point, 2 * annotation, annotation});
            else
            {
                const float length = std::min(b.width, b.height) / 4;
                for (int side : {0, 1})
                    for (int up : {0, 1})
                    {
                        const float x = b.x + side * b.width, y = b.y + up * b.height, sx = side ? -1.f : 1.f,
                                    sy = up ? -1.f : 1.f;
                        appendLine({x + sx * length, y}, {x, y});
                        appendLine({x, y}, {x, y + sy * length});
                    }
            }
            if (s.options.tracers)
                appendLine({v.x + v.width / 2, v.y + v.height}, marker.point);
        }
        for (unsigned i = 0; i < entityBrushes.size(); ++i)
            if (!markers[i].empty() || !points[i].empty())
            {
                const auto outline = espGeometry(factory.Get(), markers[i], points[i], {}, 3);
                const auto fill = espGeometry(factory.Get(), markers[i], points[i], {});
                context->FillGeometry(outline.Get(), outlineBrush.Get());
                context->FillGeometry(fill.Get(), entityBrushes[i].Get());
            }
        std::unordered_set<std::uint64_t> liveLabels;
        for (const auto &marker : s.markers)
        {
            auto brush = entityBrushes[marker.group].Get();
            const auto b = marker.bounds;
            std::wstring label;
            const auto append = [&](std::wstring part) {
                if (part.empty())
                    return;
                if (!label.empty())
                    label += L" · ";
                label += part;
            };
            if (s.options.names)
                append(marker.name);
            if (s.options.distance && marker.distanceMeters)
            {
                std::wostringstream distance;
                distance << std::fixed << std::setprecision(0) << *marker.distanceMeters << L'm';
                append(distance.str());
            }
            if (s.options.health && marker.health && std::isfinite(*marker.health) && *marker.health >= 0)
            {
                const auto number = [](double value) {
                    std::wostringstream text;
                    text << std::fixed << std::setprecision(std::abs(value - std::round(value)) < .05 ? 0 : 1) << value;
                    return text.str();
                };
                auto hp = L"HP " + number(*marker.health);
                if (marker.maxHealth && std::isfinite(*marker.maxHealth) && *marker.maxHealth >= 0)
                    hp += L"/" + number(*marker.maxHealth);
                append(std::move(hp));
            }
            append(marker.extraLabel);
            if (marker.estimated)
                append(english ? L"estimated" : L"估算");
            UINT32 emphasisStart = 0;
            if (marker.emphasis && !marker.emphasis->text.empty())
            {
                if (!label.empty())
                    label += L"\n";
                emphasisStart = static_cast<UINT32>(label.size());
                label += marker.emphasis->text;
            }
            if (!label.empty())
            {
                liveLabels.insert(marker.id);
                auto &cached = labelLayouts[marker.id];
                // Cache unscaled layouts. Movement/FOV changes transform them;
                // they do not create a new DirectWrite layout every frame.
                const auto available = std::max(1.f, (v.width - 6) / 1.1f);
                if (!cached.layout || cached.content != label || cached.width != available)
                {
                    cached.layout.Reset();
                    check(writeFactory->CreateTextLayout(label.c_str(), static_cast<UINT32>(label.size()),
                                                         textFormat.Get(), available, 50, &cached.layout),
                          "ESP label layout");
                    cached.layout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                    check(cached.layout->GetMetrics(&cached.metrics), "ESP label metrics");
                    // Explicit multi-line query labels must fit before drawing with clipping enabled.
                    if (cached.metrics.height > cached.layout->GetMaxHeight())
                    {
                        check(cached.layout->SetMaxHeight(cached.metrics.height), "ESP label height");
                        check(cached.layout->GetMetrics(&cached.metrics), "ESP expanded label metrics");
                    }
                    cached.content = label;
                    cached.width = available;
                }
                const auto &metrics = cached.metrics;
                if (marker.emphasis)
                    targetBrush->SetColor(D2D1::ColorF(marker.emphasis->color));
                const float annotation = marker.annotationScale, labelWidth = metrics.width * annotation,
                            labelHeight = metrics.height * annotation;
                const float x = std::clamp(b.x + b.width / 2 - labelWidth / 2, v.x + 3,
                                           std::max(v.x + 3, v.x + v.width - labelWidth - 3));
                const float y = std::clamp(b.y - labelHeight - 5 * annotation, v.y + 3,
                                           std::max(v.y + 3, v.y + v.height - labelHeight - 3));
                D2D1_MATRIX_3X2_F original;
                context->GetTransform(&original);
                context->SetTransform(D2D1::Matrix3x2F::Scale(annotation, annotation) *
                                      D2D1::Matrix3x2F::Translation(x, y) * original);
                check(cached.layout->SetDrawingEffect(nullptr, {0, static_cast<UINT32>(label.size())}),
                      "ESP label effect reset");
                for (const Point offset : std::array<Point, 4>{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}})
                    context->DrawTextLayout(point(offset), cached.layout.Get(), outlineBrush.Get(),
                                            D2D1_DRAW_TEXT_OPTIONS_CLIP);
                if (marker.emphasis && !marker.emphasis->text.empty())
                    check(cached.layout->SetDrawingEffect(
                              targetBrush.Get(), {emphasisStart, static_cast<UINT32>(marker.emphasis->text.size())}),
                          "ESP emphasis color");
                context->DrawTextLayout({0, 0}, cached.layout.Get(), brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                context->SetTransform(original);
            }
        }
        std::erase_if(labelLayouts, [&](const auto &item) { return !liveLabels.contains(item.first); });
        if (s.aim)
        {
            targetBrush->SetColor(D2D1::ColorF(s.targetLocked ? palette.selected : palette.entity));
            context->DrawEllipse(D2D1::Ellipse(point(*s.aim), 8 * s.aimAnnotationScale, 8 * s.aimAnnotationScale),
                                 targetBrush.Get(), 2 * s.aimAnnotationScale);
        }
        context->PopAxisAlignedClip();
        if (!transparent && (s.summary.state != EspState::Ready || !s.summary.counts.visible))
        {
            textBrush->SetColor(D2D1::ColorF(palette.text));
            const auto message = s.summary.state == EspState::Ready
                                     ? (english ? L"No visible people in the current view" : L"当前视野没有可见人物")
                                 : s.summary.state == EspState::InvalidCamera
                                     ? (english ? L"Incomplete camera pose; waiting for a valid sample"
                                                : L"相机姿态不完整，等待当前有效样本")
                                 : s.summary.state == EspState::WorldMismatch
                                     ? (english ? L"Camera and people belong to different scenes"
                                                : L"相机与人物不属于同一场景，等待更新")
                                 : s.summary.state == EspState::WaitingLiving
                                     ? (english ? L"Camera ready; waiting for people" : L"相机已就绪，等待人物数据")
                                     : (english ? L"Waiting for Local camera data" : L"等待 Local 相机数据");
            context->DrawText(
                message, static_cast<UINT32>(wcslen(message)), textFormat.Get(),
                D2D1::RectF(v.x + 24, v.y + v.height / 2 - 12, v.x + v.width - 24, v.y + v.height / 2 + 35),
                textBrush.Get());
        }
        drawHud(s, palette);
        check(context->EndDraw(), "ESP EndDraw");
    }
    void drawHud(const EspPrimitives &s, const Palette &palette)
    {
        const auto &ring = s.bearing;
        if (ring.radius > 0)
        {
            const auto center = point(ring.center);
            hudBrush->SetColor(D2D1::ColorF(palette.background, .94f));
            context->FillEllipse(D2D1::Ellipse(center, ring.radius + 12, ring.radius + 12), hudBrush.Get());
            gridBrush->SetColor(D2D1::ColorF(palette.guide));
            context->DrawEllipse(D2D1::Ellipse(center, ring.radius, ring.radius), gridBrush.Get(), 1);
            context->DrawEllipse(D2D1::Ellipse(center, ring.radius + 8, ring.radius + 8), gridBrush.Get(), .5f);
            for (const Point direction : std::array<Point, 4>{{{0, -1}, {1, 0}, {0, 1}, {-1, 0}}})
                context->DrawLine(
                    {center.x + direction.x * (ring.radius - 5), center.y + direction.y * (ring.radius - 5)},
                    {center.x + direction.x * (ring.radius + 4), center.y + direction.y * (ring.radius + 4)},
                    gridBrush.Get(), 1);
            std::array<std::vector<Point>, EspGroupCount> groups;
            for (const auto &marker : ring.markers)
                if (marker.group < groups.size())
                    groups[marker.group].push_back(marker.point);
            for (unsigned group = 0; group < groups.size(); ++group)
                if (!groups[group].empty())
                {
                    const auto path = geometry({}, groups[group], group >= EspPreselectedGroup ? 4.f : 3.f);
                    context->DrawGeometry(path.Get(), outlineBrush.Get(), 2);
                    context->FillGeometry(path.Get(), entityBrushes[group].Get());
                }
            hudBrush->SetColor(D2D1::ColorF(ring.ready ? palette.text : palette.guide));
            const auto arrow = geometry({{{center.x - 4, center.y + 5}, {center.x, center.y - 6}},
                                         {{center.x, center.y - 6}, {center.x + 4, center.y + 5}},
                                         {{center.x + 4, center.y + 5}, {center.x - 4, center.y + 5}}},
                                        {}, 0);
            context->DrawGeometry(arrow.Get(), hudBrush.Get(), 1.5f);
        }
        const auto rate = drawFramesPerSecond ? std::to_wstring(*drawFramesPerSecond) + L" FPS" : L"— FPS";
        const auto watchers = s.spectators ? std::to_wstring(*s.spectators) : L"—";
        textBrush->SetColor(D2D1::ColorF(palette.text));
        textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        context->DrawText(rate.c_str(), static_cast<UINT32>(rate.size()), textFormat.Get(),
                          D2D1::RectF(width / scale - 200, 18, width / scale - 75, 44), textBrush.Get());
        context->DrawText(watchers.c_str(), static_cast<UINT32>(watchers.size()), textFormat.Get(),
                          D2D1::RectF(width / scale - 67, 18, width / scale - 22, 44), textBrush.Get());
        textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    }
};
D2DScenePainter::D2DScenePainter(ID2D1Factory1 *factory, ID2D1DeviceContext *context, std::span<const std::byte> font)
    : impl_(std::make_unique<Impl>(factory, context, font))
{
}
D2DScenePainter::~D2DScenePainter() = default;
void D2DScenePainter::resize(unsigned width, unsigned height, float scale)
{
    impl_->resize(width, height, scale);
}
void D2DScenePainter::draw(const Primitives &scene, const Palette &palette, bool english,
                           std::optional<std::uint64_t> drawFramesPerSecond, ID2D1Bitmap1 *meshOverlay,
                           bool transparent)
{
    impl_->english = english;
    impl_->drawFramesPerSecond = drawFramesPerSecond;
    impl_->meshOverlay = meshOverlay;
    impl_->transparent = transparent;
    impl_->context->SetTextAntialiasMode(transparent ? D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE
                                                     : D2D1_TEXT_ANTIALIAS_MODE_DEFAULT);
    impl_->draw(scene, palette);
    impl_->meshOverlay = nullptr;
}
} // namespace mousebox::render::detail
