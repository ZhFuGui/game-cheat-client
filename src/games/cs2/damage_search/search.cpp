#include "search.hpp"
#include "angular_domain.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <limits>
#include <queue>
#include <set>

namespace mousebox::games::cs2::damage_search
{
using namespace application::ports;
namespace
{
struct Patch
{
    AngularRect bounds;
    double upper = INFINITY, priority = 0;
    unsigned depth = 0;
    std::uint64_t order = 0;
};
struct Earlier
{
    bool operator()(const Patch &a, const Patch &b) const
    {
        return a.priority == b.priority ? a.order > b.order : a.priority < b.priority;
    }
};
} // namespace
Result search(const hitboxes::Pose &pose, Vec3 origin, std::uint32_t weapon, unsigned team,
              const PlayerDamageTarget &target, const BallisticsScene &scene, Limits limits, std::optional<Seed> seed,
              std::stop_token stop)
{
    Result result;
    const hitboxes::TargetIntersector targetGeometry(pose);
    const auto domain = angularDomain(pose, origin);
    if (!targetGeometry.valid() || !domain)
    {
        result.code = "hitbox-domain-unavailable";
        return result;
    }
    limits.rays = std::min(limits.rays, 1024u);
    limits.depth = std::min(limits.depth, 10u);
    if (!std::isfinite(limits.epsilon) || limits.epsilon < 0)
        limits.epsilon = .1;
    std::map<unsigned, double> upper;
    double priorityCeiling = 1;
    for (const auto &bound : scene.damageUpperBounds(weapon, team, target))
        if (std::isfinite(bound.damage) && bound.damage >= 0)
        {
            upper[bound.group] = bound.damage;
            priorityCeiling = std::max(priorityCeiling, bound.damage);
        }
    const auto patchUpper = [&](const AngularRect &rect) {
        double maximum = 0;
        for (const auto &footprint : domain->footprints)
            if (rect.overlaps(footprint.bounds))
            {
                const auto found = upper.find(footprint.group);
                if (found == upper.end())
                    return std::numeric_limits<double>::infinity();
                maximum = std::max(maximum, found->second);
            }
        return maximum;
    };
    std::priority_queue<Patch, std::vector<Patch>, Earlier> queue;
    std::uint64_t order = 0;
    const auto enqueue = [&](AngularRect rect, unsigned depth) {
        const auto bound = patchUpper(rect);
        queue.push({rect, bound, (std::isfinite(bound) ? bound : priorityCeiling * 2) / (1. + depth), depth, order++});
    };
    for (const auto &footprint : domain->footprints)
        enqueue(footprint.bounds, 0);
    std::set<AngularPoint> seen;
    std::vector<DamageImpact> impacts;
    std::vector<hitboxes::Intersection> intersections;
    const auto stopped = [&] { return stop.stop_requested() || std::chrono::steady_clock::now() >= limits.deadline; };
    const auto bestDamage = [&] { return result.best ? *result.best->prediction.healthDamage : -1.; };
    const auto prunable = [&](double bound) {
        return result.best && std::isfinite(bound) && bound <= bestDamage() + limits.epsilon;
    };
    const auto evaluate = [&] {
        if (impacts.empty() || stopped())
            return;
        const auto predictions = scene.predictImpacts(weapon, team, {origin.x, origin.y, origin.z}, impacts, target);
        result.queries += static_cast<unsigned>(impacts.size());
        for (std::size_t i = 0; i < impacts.size(); ++i)
        {
            if (i >= predictions.size() || !predictions[i].healthDamage ||
                !std::isfinite(*predictions[i].healthDamage) || *predictions[i].healthDamage < 0 ||
                predictions[i].flight.outcome == ShotOutcome::Unknown)
            {
                ++result.unknown;
                continue;
            }
            const auto &prediction = predictions[i];
            if (*prediction.healthDamage > bestDamage())
            {
                const auto local = targetGeometry.localPoint(intersections[i].id, intersections[i].point);
                if (local)
                    result.best = BestPoint{intersections[i], prediction, {intersections[i].id, *local}};
            }
        }
        impacts.clear();
        intersections.clear();
    };
    const auto geometryLimit = limits.rays * 16 + 64;
    const auto candidate = [&](AngularPoint p) {
        if (stopped() || result.queries + impacts.size() >= limits.rays || result.geometryQueries >= geometryLimit ||
            !seen.insert(p).second)
            return;
        ++result.geometryQueries;
        const auto hit = targetGeometry.first(origin, domain->endpoint(p));
        if (!hit)
            return;
        impacts.push_back({{hit->point.x, hit->point.y, hit->point.z}, hit->group});
        intersections.push_back(*hit);
        if (impacts.size() == 16)
            evaluate();
    };
    // A previous winner is only a bone-local seed; it is transformed, intersected
    // and evaluated again against this capture, never reused as an old damage value.
    if (seed)
        for (const auto &box : pose.hitboxes)
            if (box.definition->id == seed->hitbox)
                if (const auto p = domain->project(box.point(seed->local)))
                    candidate(*p);
    for (const auto &footprint : domain->footprints)
        candidate(footprint.representative);
    evaluate();
    double unresolved = 0;
    while (!queue.empty() && !stopped() && result.queries + impacts.size() < limits.rays &&
           result.geometryQueries < geometryLimit)
    {
        const auto patch = queue.top();
        queue.pop();
        if (prunable(patch.upper))
            continue;
        candidate(patch.bounds.center());
        if (patch.depth == limits.depth)
            unresolved = std::max(unresolved, patch.upper);
        else
            for (const auto &child : patch.bounds.split())
                enqueue(child, patch.depth + 1);
        // Equal coarse samples never certify a patch as clear/blocked.
    }
    evaluate();
    while (!queue.empty())
    {
        unresolved = std::max(unresolved, queue.top().upper);
        queue.pop();
    }
    if (std::isfinite(unresolved))
        result.remainingUpperBound = unresolved;
    result.cancelled = stop.stop_requested();
    result.converged = !result.cancelled && prunable(unresolved);
    result.code = result.cancelled   ? "cancelled"
                  : result.converged ? "bounded-best"
                  : result.best      ? "budget-best"
                                     : "no-known-impact";
    return result;
}
} // namespace mousebox::games::cs2::damage_search
