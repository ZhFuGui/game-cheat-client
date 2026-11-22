#pragma once
#include "domain/math.hpp"
#include "domain/source_clock.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mousebox::scene
{
using domain::Vec3;
enum class FactStatus
{
    Unknown,
    Unavailable,
    Current,
    Stale,
    Invalid
};
enum class ScaleSource
{
    Unknown,
    Source,
    GameProfile
};
enum class FovAxis
{
    Horizontal,
    Vertical
};
struct SourceStamp
{
    std::string gameId, sourceId, clockDomain;
    std::uint64_t session = 0, epoch = 0, capture = 0, mappingRevision = 0;
    std::int64_t startedNs = 0, finishedNs = 0;
    std::optional<control::SourceClockEstimate> clock;
    FactStatus status = FactStatus::Unknown;
};
struct CoordinateSpace
{
    std::string id, convention;
    std::uint64_t originEpoch = 0;
    std::optional<double> metersPerUnit;
    ScaleSource scaleSource = ScaleSource::Unknown;
    Vec3 verticalUp{0, 0, 1};
};
struct Position
{
    Vec3 value;
    std::string spaceId;
    std::uint64_t originEpoch = 0;
};
struct CameraPose
{
    Position position;
    Vec3 forward, right, up;
    double fovDegrees = 0;
    SourceStamp positionSource, orientationSource;
    FovAxis fovAxis = FovAxis::Horizontal;
};
struct Joint
{
    unsigned sourceIndex = 0;
    Position position;
    std::int64_t startedNs = 0, finishedNs = 0;
    bool head = false;
};
struct Entity
{
    // Opaque game/source-scoped lifecycle; no cross-game numeric-address identity.
    std::string key;
    std::optional<Position> actorPosition;
    std::vector<Joint> joints;
    std::vector<std::pair<std::size_t, std::size_t>> edges;
};
struct Snapshot
{
    SourceStamp source;
    CoordinateSpace coordinates;
    std::optional<CameraPose> camera;
    std::vector<Entity> entities;
};
// Explicit conversion only. Unknown units or an unverified basis never become
// meters by assuming centimeters, a game engine, or a conventional up axis.
struct MetricTransform
{
    bool verified = false;
    std::optional<double> metersPerUnit;
    std::array<Vec3, 3> nativeAxes{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    Vec3 originMeters;
};
bool validBasis(Vec3 forward, Vec3 right, Vec3 up) noexcept;
std::optional<Vec3> toMetric(Vec3 native, const MetricTransform &) noexcept;
std::optional<Vec3> cameraSpace(const CameraPose &, const Position &) noexcept;
} // namespace mousebox::scene
