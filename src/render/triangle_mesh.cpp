#include "triangle_mesh.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace mousebox::render
{
MeshBuilder::MeshBuilder(std::array<double, 16> matrix, MeshViewport viewport, bool wireframe) : matrix_(matrix)
{
    mesh_.viewport = viewport;
    mesh_.wireframe = wireframe;
}
void MeshBuilder::triangle(const std::array<domain::Vec3, 3> &triangle, std::uint32_t rgb)
{
    if (mesh_.vertices.size() + 3 > 2000000)
        return;
    std::array<ClipVertex, 3> result;
    double nearest = nearest_;
    bool front = false;
    for (unsigned i = 0; i < 3; ++i)
    {
        const auto p = triangle[i];
        const auto &m = matrix_;
        const std::array<double, 4> clip{m[0] * p.x + m[1] * p.y + m[2] * p.z + m[3],
                                         m[4] * p.x + m[5] * p.y + m[6] * p.z + m[7], 0,
                                         m[12] * p.x + m[13] * p.y + m[14] * p.z + m[15]};
        for (const auto value : clip)
            if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max())
                return;
        if (clip[3] > std::numeric_limits<float>::min())
        {
            nearest = nearest > 0 ? std::min(nearest, clip[3]) : clip[3];
            front = true;
        }
        result[i] = {{static_cast<float>(clip[0]), static_cast<float>(clip[1]), 0, static_cast<float>(clip[3])},
                     0xff000000u | ((rgb & 0xff) << 16) | (rgb & 0xff00) | ((rgb >> 16) & 0xff)};
    }
    if (front)
    {
        mesh_.vertices.insert(mesh_.vertices.end(), result.begin(), result.end());
        nearest_ = nearest;
    }
}
TriangleMesh MeshBuilder::finish()
{
    const auto near = static_cast<float>(std::max(nearest_ * .0001, double(std::numeric_limits<float>::min())));
    for (auto &vertex : mesh_.vertices)
        vertex.position[2] = near;
    return std::move(mesh_);
}
} // namespace mousebox::render
