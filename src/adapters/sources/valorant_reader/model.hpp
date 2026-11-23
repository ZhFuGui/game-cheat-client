#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace mousebox::sources::valorant_reader
{
struct Vec3
{
    double x = 0, y = 0, z = 0;
};
struct Camera
{
    std::optional<Vec3> position, rotation;
    unsigned projection = 0;
    std::optional<double> fov, aspect, orthoWidth, engineTime;
    std::optional<bool> constrainAspect;
};
using Skeleton = std::array<std::optional<Vec3>, 22>;
inline constexpr std::array<int, 22> JointParents{-1, 0,  0,  2, 3,  4,  3,  6, 7,  8,  3,
                                                  10, 11, 12, 1, 14, 15, 16, 1, 18, 19, 20};
struct Capture
{
    std::uint64_t epoch = 0, sequence = 0, configuration = 0, periodNs = 0, clockRevision = 0;
    std::optional<std::uint64_t> started, readFinished, finished;
};
struct MapIdentity
{
    std::string packagePath, worldName;
};
struct PositionContext
{
    std::optional<MapIdentity> map;
    unsigned localState = 0;
    std::optional<std::uint64_t> localEntity;
    std::optional<unsigned> referenceBombTeam;
};
struct Position
{
    std::optional<Vec3> world;
    std::optional<unsigned> bombTeam;
    unsigned relationship = 0;
};
// Each connection owns one independent baseline. Joining these collections does
// not turn their observation intervals into a common capture frame.
struct StreamState
{
    unsigned stream = 0;
    Capture capture;
    bool baseline = false;
    std::map<std::uint64_t, Position> positions;
    PositionContext context;
    std::map<std::uint64_t, std::optional<Skeleton>> skeletons;
    std::optional<Camera> camera;
};
} // namespace mousebox::sources::valorant_reader
