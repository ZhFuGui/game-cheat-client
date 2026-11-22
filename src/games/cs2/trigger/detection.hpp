#pragma once
#include "games/cs2/model/snapshot.hpp"
#include "games/cs2/model/target_filter.hpp"
#include "weapon_filter.hpp"
#include "games/cs2/hitboxes/intersection.hpp"
namespace mousebox::games::cs2::trigger
{
struct Detection
{
    std::string block = "no-hit";
    std::uint32_t target = 0;
    unsigned hitbox = 0;
    Vec3 origin{}, point{};
    std::uint32_t controller = 0;
};
// Rage can conservatively treat a first-hit player with incomplete team/alive
// information as an excluded blocker. Ordinary Triggerbot retains its policy.
enum class UnknownTargetPolicy
{
    Ignore,
    Block
};
struct DetectionOptions
{
    double insetFraction = 0, horizonMs = 0;
    std::optional<float> sniperInaccuracyLimit;
    bool weaponAccuracyEvaluation = false;
};
// Immutable per-capture geometry; time advances without rebuilding every pose.
class DetectionScene
{
    struct Target
    {
        std::uint32_t controller, pawn;
        std::optional<Vec3> velocity;
        std::string block;
        hitboxes::TargetIntersector geometry;
    };
    std::string block_;
    Vec3 origin_{};
    Angles direction_{};
    std::optional<Vec3> velocity_;
    bool sniper_ = false;
    std::optional<double> sniperInaccuracy_;
    std::optional<Angles> offset_;
    std::optional<AimPunchState> punch_;
    std::vector<Target> targets_;

  public:
    DetectionScene(const Snapshot &, WeaponFilter = {}, TargetFilter = {},
                   UnknownTargetPolicy = UnknownTargetPolicy::Ignore);
    Detection detect(DetectionOptions = {}) const;
};
Detection detect(const Snapshot &, WeaponFilter = {}, TargetFilter = {},
                 UnknownTargetPolicy = UnknownTargetPolicy::Ignore, DetectionOptions = {});
} // namespace mousebox::games::cs2::trigger
