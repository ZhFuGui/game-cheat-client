#include "ballistics.hpp"
#include <cs2-external-autowall/api.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <xmmintrin.h>

namespace mousebox::adapters::traceline::detail
{
using namespace application::ports;
class ProfilePackage
{
  public:
    std::shared_ptr<const platform::DynamicLibrary> library;
    decltype(&cs2tl_destroy_profiles) destroyProfiles;
    decltype(&cs2tl_create_ballistics) create;
    decltype(&cs2tl_destroy_ballistics) destroy;
    decltype(&cs2tl_calculate_remaining_damage) calculate;
    decltype(&cs2tl_calculate_target_damage) calculateTarget;
    using CalculateBatch = decltype(&cs2tl_calculate_remaining_damage_batch);
    CalculateBatch calculateBatch;
    cs2tl_profiles *profiles = nullptr;
    std::set<unsigned> commonModes, damageModes;
    std::map<unsigned, float> baseDamage;
    cs2tl_damage_rules attackableRules{sizeof(cs2tl_damage_rules)};
    ProfilePackage(std::shared_ptr<const platform::DynamicLibrary> owner, const std::filesystem::path &path)
        : library(std::move(owner))
    {
        const auto symbol = [&](const char *name) { return library->symbol(name); };
        const auto load = reinterpret_cast<decltype(&cs2tl_load_profiles)>(symbol("cs2tl_load_profiles"));
        const auto info = reinterpret_cast<decltype(&cs2tl_get_profiles_info)>(symbol("cs2tl_get_profiles_info"));
        const auto at = reinterpret_cast<decltype(&cs2tl_get_weapon_profile_at)>(symbol("cs2tl_get_weapon_profile_at"));
        destroyProfiles = reinterpret_cast<decltype(destroyProfiles)>(symbol("cs2tl_destroy_profiles"));
        create = reinterpret_cast<decltype(create)>(symbol("cs2tl_create_ballistics"));
        destroy = reinterpret_cast<decltype(destroy)>(symbol("cs2tl_destroy_ballistics"));
        calculate = reinterpret_cast<decltype(calculate)>(symbol("cs2tl_calculate_remaining_damage"));
        calculateTarget = reinterpret_cast<decltype(calculateTarget)>(symbol("cs2tl_calculate_target_damage"));
        calculateBatch = reinterpret_cast<CalculateBatch>(symbol("cs2tl_calculate_remaining_damage_batch"));
        const auto bytes = path.u8string();
        const std::string utf8(bytes.begin(), bytes.end());
        cs2tl_error error{sizeof(error)};
        if (load(utf8.c_str(), &profiles, &error) != CS2TL_OK || !profiles)
            throw std::runtime_error("穿透武器与材质参数包无法加载");
        try
        {
            const auto defaults =
                reinterpret_cast<decltype(&cs2tl_default_damage_rules)>(symbol("cs2tl_default_damage_rules"));
            if (defaults(&attackableRules, &error) != CS2TL_OK)
                throw std::runtime_error("穿透伤害规则无法加载");
            attackableRules.teammates_are_enemies = 1;
            attackableRules.flags &= ~CS2TL_DAMAGE_RULES_LIVE_SNAPSHOT;
            cs2tl_profiles_info data{sizeof(data)};
            if (info(profiles, &data, &error) != CS2TL_OK || data.weapon_profile_count > 4096 ||
                data.ruleset_version != CS2TL_RULESET_CS2_STATIC_V1)
                throw std::runtime_error("穿透参数包规则不兼容");
            std::map<unsigned, std::map<unsigned, cs2tl_weapon_profile>> weapons;
            for (unsigned i = 0; i < data.weapon_profile_count; ++i)
            {
                cs2tl_weapon_profile weapon{sizeof(weapon)};
                if (at(profiles, i, &weapon, &error) != CS2TL_OK ||
                    !weapons[weapon.weapon_id].emplace(weapon.fire_mode, weapon).second)
                    throw std::runtime_error("穿透武器参数索引无效");
            }
            const auto parameters = [](const cs2tl_weapon_profile &p) {
                return std::tuple(p.ammo_id, p.base_damage, p.max_range, p.range_modifier, p.penetration_power,
                                  p.pellets_per_shot);
            };
            for (const auto &[id, modes] : weapons)
                if (modes.contains(CS2TL_FIRE_PRIMARY) && modes.contains(CS2TL_FIRE_ALTERNATE) &&
                    parameters(modes.at(CS2TL_FIRE_PRIMARY)) == parameters(modes.at(CS2TL_FIRE_ALTERNATE)))
                {
                    commonModes.insert(id);
                    const auto &a = modes.at(CS2TL_FIRE_PRIMARY), &b = modes.at(CS2TL_FIRE_ALTERNATE);
                    if (a.armor_ratio == b.armor_ratio && a.headshot_multiplier == b.headshot_multiplier)
                    {
                        damageModes.insert(id);
                        if (std::isfinite(a.range_modifier) && a.range_modifier > 0 && a.range_modifier <= 1 &&
                            std::isfinite(a.base_damage) && a.base_damage > 0 && std::isfinite(a.armor_ratio) &&
                            a.armor_ratio >= 0 && a.armor_ratio <= 2)
                            baseDamage.emplace(id, a.base_damage);
                    }
                }
            // The reviewed package has equal relevant values in both modes.
            // A later package with differing modes needs a verified runtime selector.
        }
        catch (...)
        {
            destroyProfiles(std::exchange(profiles, nullptr));
            throw;
        }
    }
    ~ProfilePackage()
    {
        destroyProfiles(profiles);
    }
};
namespace
{
class BoundScene final : public BallisticsScene
{
    std::shared_ptr<const ProfilePackage> profiles_;
    std::shared_ptr<const VisibilityScene> geometry_;
    cs2tl_ballistics *context_ = nullptr;
    static bool valid(domain::Vec3 p)
    {
        constexpr auto limit = std::numeric_limits<float>::max();
        return domain::finite(p) && std::abs(p.x) <= limit && std::abs(p.y) <= limit && std::abs(p.z) <= limit;
    }
    static cs2tl_remaining_damage_request request(unsigned weapon, domain::Vec3 origin, domain::Vec3 target)
    {
        return {sizeof(cs2tl_remaining_damage_request),
                weapon,
                CS2TL_FIRE_PRIMARY,
                0,
                {sizeof(cs2tl_segment),
                 0,
                 {static_cast<float>(origin.x), static_cast<float>(origin.y), static_cast<float>(origin.z)},
                 {static_cast<float>(target.x), static_cast<float>(target.y), static_cast<float>(target.z)},
                 0,
                 0}};
    }
    static ShotPrediction unpack(cs2tl_status code, const cs2tl_shot_prediction &prediction)
    {
        ShotPrediction result;
        if (code != CS2TL_OK ||
            (prediction.outcome != CS2TL_SHOT_REACHED && prediction.outcome != CS2TL_SHOT_STOPPED) ||
            !std::isfinite(prediction.remaining_damage) || prediction.remaining_damage < 0)
        {
            result.code = code == CS2TL_DATA_NOT_FOUND ? "unknown-material" : "incomplete-path";
            return result;
        }
        result.outcome = prediction.outcome == CS2TL_SHOT_REACHED ? ShotOutcome::Reached : ShotOutcome::Stopped;
        result.remainingDamage = prediction.remaining_damage;
        result.penetrations = prediction.penetration_count;
        result.staticScene = (prediction.flags & CS2TL_PREDICTION_STATIC_SCENE) != 0;
        result.liveValidated = !(prediction.flags & CS2TL_PREDICTION_NOT_LIVE_VALIDATED);
        result.buildMatches = !(prediction.flags & CS2TL_PREDICTION_BUILD_MISMATCH);
        result.interactionMetadataComplete = !(prediction.flags & CS2TL_PREDICTION_INTERACTION_FLAGS_UNAVAILABLE);
        result.nativeStaticEvents = (prediction.flags & CS2TL_PREDICTION_NATIVE_STATIC_EVENTS) != 0;
        return result;
    }
    ImpactDamagePrediction targetDamage(unsigned weapon, unsigned team, unsigned group,
                                        const PlayerDamageTarget &target, ShotPrediction flight,
                                        const cs2tl_damage_rules *rules = nullptr) const
    {
        ImpactDamagePrediction result;
        if (!profiles_->damageModes.contains(weapon) || target.health <= 0 || target.armor < 0 || group < 1 ||
            group > 8 || (target.team != 2 && target.team != 3) || (team != 2 && team != 3))
        {
            result.code = "target-data-unavailable";
            return result;
        }
        result.flight = std::move(flight);
        if (result.flight.outcome == ShotOutcome::Stopped)
        {
            result.healthDamage = 0;
            result.healthLossMin = 0;
            result.healthLossMax = 0;
            return result;
        }
        if (result.flight.outcome != ShotOutcome::Reached || !result.flight.remainingDamage)
        {
            result.code = result.flight.code;
            return result;
        }
        const cs2tl_target_state state{sizeof(state),
                                       group,
                                       target.health,
                                       target.armor,
                                       target.team,
                                       target.helmet ? CS2TL_TARGET_HAS_HELMET : 0u,
                                       0,
                                       0};
        const cs2tl_target_damage_request input{
            sizeof(input), weapon, CS2TL_FIRE_PRIMARY, team, static_cast<float>(*result.flight.remainingDamage), 0,
            &state,        rules};
        cs2tl_target_damage_result damage{sizeof(damage)};
        const auto status = profiles_->calculateTarget(profiles_->profiles, &input, &damage, nullptr);
        if (status != CS2TL_OK ||
            (damage.outcome != CS2TL_DAMAGE_EVALUATED && damage.outcome != CS2TL_DAMAGE_BLOCKED) ||
            !std::isfinite(damage.health_damage) || damage.health_damage < 0 ||
            damage.health_loss_min > damage.health_loss_max ||
            damage.health_loss_max > static_cast<unsigned>(target.health))
        {
            result.code = "target-damage-unavailable";
            return result;
        }
        result.healthDamage = damage.health_damage;
        result.healthLossMin = damage.health_loss_min;
        result.healthLossMax = damage.health_loss_max;
        result.assumedRules = rules || (damage.flags & CS2TL_PREDICTION_ASSUMED_DAMAGE_RULES) != 0;
        return result;
    }

  public:
    BoundScene(std::shared_ptr<const ProfilePackage> profiles, cs2tl_scene *scene,
               std::shared_ptr<const VisibilityScene> geometry)
        : profiles_(std::move(profiles)), geometry_(std::move(geometry))
    {
        cs2tl_error error{sizeof(error)};
        if (profiles_->create(scene, profiles_->profiles, &context_, &error) != CS2TL_OK || !context_)
            throw std::runtime_error("当前静态场景无法绑定穿透参数");
    }
    ~BoundScene() override
    {
        profiles_->destroy(context_);
    }
    ShotPrediction predict(std::uint32_t weapon, domain::Vec3 origin, domain::Vec3 target) const override
    {
        ShotPrediction result;
        if (!profiles_->commonModes.contains(weapon))
        {
            result.code = "weapon-profile-unavailable";
            return result;
        }
        if (!valid(origin) || !valid(target))
        {
            result.code = "invalid-segment";
            return result;
        }
        const auto input = request(weapon, origin, target);
        cs2tl_shot_prediction prediction{sizeof(prediction)};
        return unpack(profiles_->calculate(context_, &input, &prediction, nullptr), prediction);
    }
    HeadDamagePrediction predictHead(std::uint32_t weapon, unsigned attackerTeam, domain::Vec3 origin,
                                     domain::Vec3 impact, const PlayerDamageTarget &target) const override
    {
        return predictImpact(weapon, attackerTeam, origin, {impact, CS2TL_HITGROUP_HEAD}, target);
    }
    ImpactDamagePrediction predictImpact(unsigned weapon, unsigned team, domain::Vec3 origin, DamageImpact impact,
                                         const PlayerDamageTarget &target) const override
    {
        return targetDamage(weapon, team, impact.group, target, predict(weapon, origin, impact.point));
    }
    std::vector<ImpactDamagePrediction> predictImpacts(unsigned weapon, unsigned team, domain::Vec3 origin,
                                                       std::span<const DamageImpact> impacts,
                                                       const PlayerDamageTarget &target) const override
    {
        return predictImpactsWithRules(weapon, team, origin, impacts, target, {});
    }
    std::vector<ImpactDamagePrediction> predictImpactsWithRules(unsigned weapon, unsigned team, domain::Vec3 origin,
                                                                std::span<const DamageImpact> impacts,
                                                                const PlayerDamageTarget &target,
                                                                DamageRuleOverrides rules) const override
    {
        if (!profiles_->damageModes.contains(weapon) || !valid(origin))
            return rules.teammatesAreEnemies ? std::vector<ImpactDamagePrediction>(impacts.size())
                                             : BallisticsScene::predictImpacts(weapon, team, origin, impacts, target);
        const auto overrideRules = rules.teammatesAreEnemies ? &profiles_->attackableRules : nullptr;
        std::vector<ImpactDamagePrediction> result(impacts.size());
        // A caller-owned bounded batch; no native queue or retained request pointers.
        for (std::size_t offset = 0; offset < impacts.size(); offset += 16)
        {
            const auto count = std::min<std::size_t>(16, impacts.size() - offset);
            std::vector<cs2tl_remaining_damage_request> requests;
            std::vector<std::size_t> indices;
            for (std::size_t i = 0; i < count; ++i)
                if (valid(impacts[offset + i].point))
                {
                    requests.push_back(request(weapon, origin, impacts[offset + i].point));
                    indices.push_back(offset + i);
                }
                else
                    result[offset + i].code = "invalid-segment";
            std::vector<cs2tl_shot_prediction> predictions(requests.size());
            for (auto &p : predictions)
                p.struct_size = sizeof(p);
            std::vector<cs2tl_status> statuses(requests.size());
            const auto status =
                profiles_->calculateBatch(context_, requests.data(), static_cast<unsigned>(requests.size()),
                                          predictions.data(), statuses.data(), nullptr, nullptr);
            for (std::size_t i = 0; i < indices.size(); ++i)
                result[indices[i]] =
                    targetDamage(weapon, team, impacts[indices[i]].group, target,
                                 unpack(status == CS2TL_OK ? statuses[i] : status, predictions[i]), overrideRules);
        }
        return result;
    }
    std::vector<DamageUpperBound> damageUpperBounds(unsigned weapon, unsigned team,
                                                    const PlayerDamageTarget &target) const override
    {
        const auto base = profiles_->baseDamage.find(weapon);
        if (base == profiles_->baseDamage.end() || (_mm_getcsr() & 0xffc0) != 0x1f80)
            return {};
        auto bare = target;
        bare.armor = 0;
        bare.helmet = false;
        ShotPrediction flight;
        flight.outcome = ShotOutcome::Reached;
        flight.remainingDamage = base->second;
        std::vector<DamageUpperBound> bounds;
        // Reviewed default-rule envelope: range/penetration cannot increase B;
        // group scaling is nonnegative and armor_ratio/2 <= 1. Either armor
        // branch only reduces S(x), so the unarmored S(B) bounds all armor states.
        // No geometric visibility or region certificate is inferred here.
        for (unsigned group = 1; group <= 8; ++group)
        {
            const auto prediction = targetDamage(weapon, team, group, bare, flight);
            if (prediction.healthDamage && prediction.flight.outcome == ShotOutcome::Reached)
                bounds.push_back(
                    {group, std::nextafter(*prediction.healthDamage, std::numeric_limits<double>::infinity())});
        }
        return bounds;
    }
};
} // namespace
std::shared_ptr<const ProfilePackage> loadProfiles(std::shared_ptr<const platform::DynamicLibrary> library,
                                                   const std::filesystem::path &path)
{
    return std::make_shared<const ProfilePackage>(std::move(library), path);
}
std::shared_ptr<const BallisticsScene> bindBallistics(std::shared_ptr<const ProfilePackage> profiles,
                                                      cs2tl_scene *scene,
                                                      std::shared_ptr<const VisibilityScene> lifetime)
{
    return std::make_shared<const BoundScene>(std::move(profiles), scene, std::move(lifetime));
}
} // namespace mousebox::adapters::traceline::detail
