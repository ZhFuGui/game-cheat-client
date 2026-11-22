#pragma once
#include "application/ports/shot_prediction.hpp"
#include "games/cs2/hitboxes/intersection.hpp"
#include <chrono>
#include <stop_token>

namespace mousebox::games::cs2::damage_search
{
struct Seed
{
    unsigned hitbox = 0;
    Vec3 local;
};
struct BestPoint
{
    hitboxes::Intersection impact;
    application::ports::ImpactDamagePrediction prediction;
    Seed seed;
};
struct Limits
{
    unsigned rays = 192, depth = 6;
    double epsilon = .1;
    std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::time_point::max();
};
struct Result
{
    std::optional<BestPoint> best;
    std::optional<double> remainingUpperBound;
    unsigned queries = 0, geometryQueries = 0, unknown = 0;
    bool converged = false, cancelled = false;
    std::string code;
};
// Pure consumer policy: first target intersection, adaptive angular patches,
// conservative upper bounds, bounded batches. No output/input or Reader access.
Result search(const hitboxes::Pose &, Vec3 origin, std::uint32_t weapon, unsigned team,
              const application::ports::PlayerDamageTarget &, const application::ports::BallisticsScene &, Limits = {},
              std::optional<Seed> = {}, std::stop_token = {});
} // namespace mousebox::games::cs2::damage_search
