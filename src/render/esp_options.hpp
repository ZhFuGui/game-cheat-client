#pragma once
#include <cstdint>
namespace mousebox::render
{
enum class EspMarker
{
    Corners,
    Point,
    Estimated
};
enum class EspAspect
{
    Wide,
    StandardWide,
    UltraWide,
    Classic,
    Viewport
};
enum class EspFovAxis
{
    Horizontal,
    Vertical
};
struct EspOptions
{
    EspMarker marker = EspMarker::Corners;
    bool names = true, distance = true, bones = false, tracers = false, crosshair = true;
    bool health = false; // Filled by games with a qualified HP source; no generic assumed maximum.
    bool teammates = true, opponents = true, bots = true, self = false;
    bool aimRange = true, targetHighlight = true;
    EspAspect aspect = EspAspect::Wide;
    EspFovAxis fovAxis = EspFovAxis::Horizontal;
    double estimatedHeightMeters = 1.8;
    double maximumDistanceMeters = 0; // Zero keeps the display distance unlimited.
    bool operator==(const EspOptions &) const = default;
};
enum class EspState
{
    WaitingCamera,
    InvalidCamera,
    WaitingLiving,
    WorldMismatch,
    Ready
};
enum class EspUnits
{
    Unknown,
    Reader,
    RadarProfile
};
struct EspCounts
{
    unsigned input = 0, visible = 0, outside = 0, behind = 0, filtered = 0, invalid = 0;
};
struct EspSummary
{
    EspState state = EspState::WaitingCamera;
    EspUnits units = EspUnits::Unknown;
    EspCounts counts;
    std::uint64_t viewCapture = 0, livingCapture = 0;
    double fov = 0;
};
} // namespace mousebox::render
