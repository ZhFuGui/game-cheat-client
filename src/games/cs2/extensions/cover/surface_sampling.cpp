#include "sampling.hpp"
#include <map>

namespace mousebox::games::cs2::extensions
{
namespace
{
Vec3 middle(Vec3 a, Vec3 b)
{
    return {(a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2};
}
class SurfaceSampler
{
    RayBudget &rays_;
    std::map<std::array<double, 3>, Visibility> points_;
    std::vector<ActorVisibility::Surface> fragments_;
    Visibility query(Vec3 p)
    {
        const std::array key{p.x, p.y, p.z};
        if (const auto found = points_.find(key); found != points_.end())
            return found->second;
        const auto value = rays_.query({p.x, p.y, p.z});
        points_.emplace(key, value);
        return value;
    }

  public:
    explicit SurfaceSampler(RayBudget &rays) : rays_(rays) {}
    void triangle(const std::array<Vec3, 3> &p, unsigned depth = 0)
    {
        if (rays_.exhausted())
        {
            ++rays_.unknown;
            fragments_.push_back({p, Visibility::Unknown});
            return;
        }
        const auto a = query(p[0]), b = query(p[1]), c = query(p[2]);
        const auto center =
            query({(p[0].x + p[1].x + p[2].x) / 3, (p[0].y + p[1].y + p[2].y) / 3, (p[0].z + p[1].z + p[2].z) / 3});
        if (a == b && b == c && c == center)
            fragments_.push_back({p, a});
        else if (depth == 1 || rays_.exhausted())
        {
            ++rays_.unknown;
            fragments_.push_back({p, Visibility::Unknown});
        }
        else
        {
            const auto ab = middle(p[0], p[1]), bc = middle(p[1], p[2]), ca = middle(p[2], p[0]);
            triangle({p[0], ab, ca}, depth + 1);
            triangle({ab, p[1], bc}, depth + 1);
            triangle({ca, bc, p[2]}, depth + 1);
            triangle({ab, bc, ca}, depth + 1);
        }
    }
    std::vector<ActorVisibility::Surface> finish()
    {
        return std::move(fragments_);
    }
};
} // namespace
std::vector<ActorVisibility::Surface> sampleHitbox(RayBudget &rays, std::span<const std::array<Vec3, 3>> triangles)
{
    SurfaceSampler sample(rays);
    for (const auto &triangle : triangles)
        sample.triangle(triangle);
    return sample.finish();
}
} // namespace mousebox::games::cs2::extensions
