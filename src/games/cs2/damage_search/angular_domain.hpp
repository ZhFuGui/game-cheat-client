#pragma once
#include "games/cs2/hitboxes/pose.hpp"
#include <optional>

namespace mousebox::games::cs2::damage_search
{
struct AngularPoint
{
    double u = 0, v = 0;
    auto operator<=>(const AngularPoint &) const = default;
};
struct AngularRect
{
    double left = 0, bottom = 0, right = 0, top = 0;
    AngularPoint center() const;
    bool overlaps(const AngularRect &) const;
    std::array<AngularRect, 4> split() const;
};
struct Footprint
{
    unsigned id = 0, group = 0;
    AngularRect bounds;
    AngularPoint representative;
};
// Conservative projection of the complete transformed local bounds. The rectangle
// includes the curved hitbox silhouette; it does not certify any cover visibility.
struct AngularDomain
{
    Vec3 origin, forward, right, up;
    double rayLength = 0;
    std::vector<Footprint> footprints;
    Vec3 endpoint(AngularPoint) const;
    std::optional<AngularPoint> project(Vec3) const;
};
std::optional<AngularDomain> angularDomain(const hitboxes::Pose &, Vec3 origin);
} // namespace mousebox::games::cs2::damage_search
