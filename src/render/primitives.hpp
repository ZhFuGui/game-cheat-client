#pragma once
#include "esp_options.hpp"
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mousebox::render
{
struct TriangleMesh;
struct Palette
{
    std::uint32_t background = 0x101a25, grid = 0x1b2a38, guide = 0x365366, text = 0x8499a9, entity = 0x50c9bd,
                  selected = 0xefbb68;
    bool operator==(const Palette &) const = default;
};
struct Point
{
    float x = 0, y = 0;
};
struct Circle
{
    Point center;
    float radius = 0;
    float strokeWidth = 0;
};
struct Line
{
    Point a, b;
};
struct ScaledLine
{
    Point a, b;
    float widthA = 0, widthB = 0;
};
struct BearingMarker
{
    Point point;
    unsigned group = 0;
    std::uint64_t id = 0;
};
struct BearingRing
{
    Point center;
    float radius = 0;
    bool ready = false;
    std::vector<BearingMarker> markers;
};
struct EspRect
{
    float x = 0, y = 0, width = 0, height = 0;
};
struct EspMarkerPrimitive
{
    struct Emphasis
    {
        std::wstring text;
        std::uint32_t color = 0xffffff;
    };
    unsigned group = 0;
    Point point;
    EspRect bounds;
    bool estimated = false;
    std::wstring name;
    std::optional<double> distanceMeters;
    std::uint64_t id = 0;
    float annotationScale = 1;
    std::optional<double> health, maxHealth;
    std::wstring extraLabel;
    std::optional<Emphasis> emphasis;
};
inline constexpr std::array<std::uint32_t, 4> EspColors{0xffffff, 0x4ade80, 0xff4040, 0xff9f43};
inline constexpr unsigned EspPreselectedGroup = 4, EspLockedGroup = 5, EspGroupCount = 6;
// Optional typed decoration batches. Color is independent of team/selection
// groups; the GPU layer does not know which game extension produced a batch.
struct StyledEspGeometry
{
    std::uint32_t color = 0;
    std::vector<ScaledLine> skeletons;
    std::vector<Circle> dots, heads;
};
struct EspPrimitives
{
    std::shared_ptr<const TriangleMesh> mesh;
    BearingRing bearing;
    std::optional<unsigned> spectators;
    EspRect viewport;
    EspOptions options;
    EspSummary summary;
    std::array<std::vector<ScaledLine>, EspGroupCount> skeletons;
    std::array<std::vector<Circle>, EspGroupCount> dots;
    std::array<std::vector<Circle>, EspGroupCount> heads;
    std::vector<StyledEspGeometry> decorations;
    std::vector<EspMarkerPrimitive> markers;
    std::optional<Point> aim;
    std::optional<float> aimRangeRadius;
    bool targetLocked = false;
    float aimAnnotationScale = 1;
};
struct Primitives
{
    std::array<std::vector<Line>, 2> skeletons;
    std::array<std::vector<Point>, 2> dots, heads;
    std::optional<Point> aim;
    unsigned visiblePoints = 0;
    std::wstring message, title;
    std::shared_ptr<const EspPrimitives> esp;
};
} // namespace mousebox::render
