#pragma once
#include "domain/math.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mousebox::application::ports
{
enum class ShotOutcome
{
    Unknown,
    Reached,
    Stopped
};
struct ShotPrediction
{
    ShotOutcome outcome = ShotOutcome::Unknown;
    std::optional<double> remainingDamage;
    unsigned penetrations = 0;
    std::string code;
    bool staticScene = true, liveValidated = false, buildMatches = true, interactionMetadataComplete = false;
    bool nativeStaticEvents = false;
};
struct PlayerDamageTarget
{
    int health = 0, armor = 0;
    unsigned team = 0;
    bool helmet = false;
    bool operator==(const PlayerDamageTarget &) const = default;
};
struct ImpactDamagePrediction
{
    ShotPrediction flight;
    std::optional<double> healthDamage;
    std::optional<unsigned> healthLossMin, healthLossMax;
    bool assumedRules = true, assumedVulnerability = true;
    std::string code;
};
using HeadDamagePrediction = ImpactDamagePrediction;
struct DamageImpact
{
    domain::Vec3 point;
    unsigned group = 0;
};
// Explicit conditional rules, not a claim about the server's current settings.
struct DamageRuleOverrides
{
    bool teammatesAreEnemies = false;
};
struct DamageUpperBound
{
    unsigned group = 0;
    double damage = 0;
};
// Read-only single-bullet path and explicit target-impact evaluation.
class BallisticsScene
{
  public:
    virtual ~BallisticsScene() = default;
    virtual ShotPrediction predict(std::uint32_t weaponProfile, domain::Vec3 origin, domain::Vec3 target) const = 0;
    // Conditional single-bullet estimate at the first verified head-surface impact.
    // Default preserves providers which support only remaining projectile energy.
    virtual HeadDamagePrediction predictHead(std::uint32_t, unsigned, domain::Vec3, domain::Vec3,
                                             const PlayerDamageTarget &) const
    {
        return {};
    }
    // Explicit first impact and native hit group. No target selection or search policy.
    virtual ImpactDamagePrediction predictImpact(std::uint32_t, unsigned, domain::Vec3, DamageImpact,
                                                 const PlayerDamageTarget &) const
    {
        return {};
    }
    virtual std::vector<ImpactDamagePrediction> predictImpacts(std::uint32_t weapon, unsigned team, domain::Vec3 origin,
                                                               std::span<const DamageImpact> impacts,
                                                               const PlayerDamageTarget &target) const
    {
        std::vector<ImpactDamagePrediction> results;
        results.reserve(impacts.size());
        for (const auto &impact : impacts)
            results.push_back(predictImpact(weapon, team, origin, impact, target));
        return results;
    }
    virtual std::vector<ImpactDamagePrediction> predictImpactsWithRules(std::uint32_t weapon, unsigned team,
                                                                        domain::Vec3 origin,
                                                                        std::span<const DamageImpact> impacts,
                                                                        const PlayerDamageTarget &target,
                                                                        DamageRuleOverrides rules) const
    {
        if (!rules.teammatesAreEnemies)
            return predictImpacts(weapon, team, origin, impacts, target);
        return std::vector<ImpactDamagePrediction>(impacts.size());
    }
    // Conservative unattenuated bounds in this provider's model. Missing means unknown, never zero.
    virtual std::vector<DamageUpperBound> damageUpperBounds(std::uint32_t, unsigned, const PlayerDamageTarget &) const
    {
        return {};
    }
};
class BallisticsProvider
{
  public:
    virtual ~BallisticsProvider() = default;
    virtual bool available() const noexcept = 0;
    virtual std::shared_ptr<const BallisticsScene> loadBallistics(std::string_view map, std::string_view resource,
                                                                  unsigned gameBuild) = 0;
};
} // namespace mousebox::application::ports
