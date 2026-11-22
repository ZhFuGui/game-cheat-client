#pragma once
#include "pose.hpp"
namespace mousebox::games::cs2::hitboxes
{
struct Intersection
{
    Vec3 point;
    unsigned group = 0, id = 0;
    double fraction = 0;
};
// Prepared once per current target pose; reused by all candidate rays.
class TargetIntersector
{
    struct Entry
    {
        const Definition *definition;
        Vec3 origin, yz, zx, xy;
        double determinant;
        Vec3 inverse(Vec3) const;
    };
    std::vector<Entry> entries_;
    bool valid_ = false;

  public:
    explicit TargetIntersector(const Pose &);
    bool valid() const noexcept
    {
        return valid_;
    }
    std::optional<Intersection> first(Vec3 start, Vec3 end, double insetFraction = 0) const;
    std::optional<Vec3> localPoint(unsigned id, Vec3 world) const;
    // Relative ray proximity to the complete posed body: 1 intersects, outside
    // falls continuously by the minimum relative shape expansion, behind is 0.
    std::optional<double> directionProximity(Vec3 start, Vec3 end) const;
};
// Complete current pose only. Uses the inverse of the exact raw CTransform,
// including signed scale and non-normalized near-unit quaternion semantics.
std::optional<Intersection> firstIntersection(const Pose &, Vec3 start, Vec3 end);
} // namespace mousebox::games::cs2::hitboxes
