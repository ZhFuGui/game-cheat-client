#pragma once
#include "result.hpp"
#include <stop_token>
#include <span>

namespace mousebox::games::cs2::extensions
{
class RayBudget
{
    const application::ports::VisibilityScene &scene_;
    domain::Vec3 origin_;
    unsigned remaining_;
    std::stop_token stop_;

  public:
    unsigned queries = 0, unknown = 0;
    RayBudget(const application::ports::VisibilityScene &scene, domain::Vec3 origin, unsigned maximum,
              std::stop_token stop)
        : scene_(scene), origin_(origin), remaining_(maximum), stop_(stop)
    {
    }
    Visibility query(domain::Vec3 target);
    bool exhausted() const noexcept
    {
        return !remaining_ || stop_.stop_requested();
    }
};
// Endpoints alone cannot classify a bone crossing the edge of a cover object.
// Interior probes and bounded transition refinement produce world-line spans.
std::vector<Span> sampleBone(RayBudget &, domain::Vec3 a, domain::Vec3 b, Visibility atA, Visibility atB);
std::vector<ActorVisibility::Surface> sampleHitbox(RayBudget &, std::span<const std::array<Vec3, 3>>);
} // namespace mousebox::games::cs2::extensions
