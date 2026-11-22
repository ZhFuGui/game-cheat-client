#include "geometry.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>

namespace mousebox::games::cs2::hitboxes
{
namespace
{
Vec3 add(Vec3 a, Vec3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
Vec3 multiply(Vec3 a, double s)
{
    return {a.x * s, a.y * s, a.z * s};
}
Vec3 difference(Vec3 a, Vec3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
Vec3 cross(Vec3 a, Vec3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
double length(Vec3 a)
{
    return std::hypot(a.x, a.y, a.z);
}
} // namespace
std::vector<Triangle> localSurface(const Definition &d)
{
    std::vector<Triangle> result;
    if (d.shape == Shape::Box)
    {
        std::array<Vec3, 8> points;
        for (unsigned i = 0; i < points.size(); ++i)
            points[i] = {i & 1 ? d.b.x : d.a.x, i & 2 ? d.b.y : d.a.y, i & 4 ? d.b.z : d.a.z};
        constexpr unsigned indices[][3] = {{0, 2, 1}, {1, 2, 3}, {4, 5, 6}, {5, 7, 6}, {0, 1, 4}, {1, 5, 4},
                                           {2, 6, 3}, {3, 6, 7}, {0, 4, 2}, {2, 4, 6}, {1, 3, 5}, {3, 7, 5}};
        for (const auto &triangle : indices)
            result.push_back({points[triangle[0]], points[triangle[1]], points[triangle[2]]});
        return result;
    }
    if (!std::isfinite(d.radius) || d.radius <= 0)
        return result;
    const auto axis = d.shape == Shape::Sphere ? Vec3{} : difference(d.b, d.a);
    const auto extent = length(axis);
    const auto direction = extent > 0 ? multiply(axis, 1 / extent) : Vec3{0, 0, 1};
    auto u = cross(direction, std::abs(direction.z) < .8 ? Vec3{0, 0, 1} : Vec3{0, 1, 0});
    u = multiply(u, 1 / length(u));
    const auto v = cross(direction, u);
    constexpr unsigned sectors = 12, hemispheres = 3;
    std::vector<std::array<Vec3, sectors>> rings;
    for (unsigned end = 0; end < 2; ++end)
        for (unsigned latitude = 0; latitude <= hemispheres; ++latitude)
        {
            const auto theta = (end ? 0 : -std::numbers::pi / 2) + latitude * std::numbers::pi / (2 * hemispheres);
            const auto center = add(d.a, multiply(direction, end ? extent : 0));
            std::array<Vec3, sectors> ring;
            for (unsigned longitude = 0; longitude < sectors; ++longitude)
            {
                const auto phi = longitude * 2 * std::numbers::pi / sectors;
                ring[longitude] = add(center, add(multiply(direction, std::sin(theta) * d.radius),
                                                  add(multiply(u, std::cos(phi) * std::cos(theta) * d.radius),
                                                      multiply(v, std::sin(phi) * std::cos(theta) * d.radius))));
            }
            rings.push_back(ring);
        }
    for (std::size_t row = 0; row + 1 < rings.size(); ++row)
        for (unsigned col = 0; col < sectors; ++col)
        {
            const auto next = (col + 1) % sectors;
            for (const Triangle &triangle : {Triangle{rings[row][col], rings[row + 1][col], rings[row][next]},
                                             Triangle{rings[row][next], rings[row + 1][col], rings[row + 1][next]}})
                if (length(cross(difference(triangle[1], triangle[0]), difference(triangle[2], triangle[0]))) >
                    d.radius * d.radius * 1e-12)
                    result.push_back(triangle);
        }
    return result;
}
std::vector<Triangle> worldSurface(const PosedHitbox &posed)
{
    // Reviewed resource shapes are immutable and shared by all 80 agent models.
    static const auto templates = [] {
        std::map<const Definition *, std::vector<Triangle>> result;
        for (const auto &model : reviewedModels())
            for (const auto &definition : model.definitions)
                if (!result.contains(&definition))
                    result.emplace(&definition, localSurface(definition));
        return result;
    }();
    const auto cached = templates.find(posed.definition);
    auto result = cached == templates.end() ? localSurface(*posed.definition) : cached->second;
    for (auto &triangle : result)
        for (auto &point : triangle)
            point = posed.point(point);
    return result;
}
} // namespace mousebox::games::cs2::hitboxes
