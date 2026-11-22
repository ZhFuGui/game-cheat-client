#pragma once
#include "domain/math.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace mousebox::render
{
struct ClipVertex
{
    std::array<float, 4> position;
    std::uint32_t rgba = 0;
};
struct MeshViewport
{
    float x = 0, y = 0, width = 0, height = 0;
    bool operator==(const MeshViewport &) const = default;
};
struct TriangleMesh
{
    std::vector<ClipVertex> vertices;
    MeshViewport viewport;
    bool wireframe = false;
};
// A perspective projection's x/y/w are sufficient for independent mesh depth.
// Reversed depth near/w is monotone in the same camera depth as the projection;
// it does not depend on a game's normal/reversed clip-Z convention.
class MeshBuilder
{
    std::array<double, 16> matrix_;
    TriangleMesh mesh_;
    double nearest_ = 0;

  public:
    MeshBuilder(std::array<double, 16>, MeshViewport, bool wireframe);
    void triangle(const std::array<domain::Vec3, 3> &, std::uint32_t rgb);
    TriangleMesh finish();
};
} // namespace mousebox::render
