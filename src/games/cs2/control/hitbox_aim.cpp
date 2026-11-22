#include "hitbox_aim.hpp"
#include "games/cs2/hitboxes/frame.hpp"
#include "pipeline/angles.hpp"
#include <array>
#include <stdexcept>

namespace mousebox::games::cs2
{
namespace
{
using hitboxes::PointDistribution;
struct AnatomicalHitbox
{
    unsigned id, group;
};
// From the reviewed cstrike resource set: head_0, neck_0, spine_3,
// spine_0 (abdomen) and pelvis. The resource owns the actual bone indices.
constexpr std::array<AnatomicalHitbox, 5> AnatomicalHitboxes{{{0, 1}, {1, 8}, {6, 2}, {3, 3}, {2, 3}}};

std::uint64_t mix(std::uint64_t value) noexcept
{
    value += 0x9E3779B97F4A7C15ULL;
    value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
    value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
    return value ^ (value >> 31);
}
std::array<double, 5> unitVariates(std::uint64_t seed, TargetKey target, unsigned hitbox) noexcept
{
    const auto id = (static_cast<std::uint64_t>(target.controller) << 32) | target.pawn;
    std::array<double, 5> result;
    auto word = seed ^ mix(id) ^ mix(hitbox);
    for (auto &u : result)
    {
        word = mix(word);
        // The midpoint of a 52-bit step is strictly between zero and one.
        u = (static_cast<double>(word >> 12) + .5) / 4503599627370496.0;
    }
    return result;
}

std::optional<hitboxes::PosedHitbox> aimVolume(const Snapshot &frame, const Body &body, unsigned part)
{
    if (part >= AnatomicalHitboxes.size())
        return {};
    const auto spec = AnatomicalHitboxes[part];
    auto posed = hitboxes::fromBody(frame, body, spec.id);
    if (!posed || posed->definition->group != spec.group ||
        (!posed->definition->translationOnly && *posed->bone.scale == 0))
        return {};
    return posed;
}

std::optional<AimCandidatePoint> drawPoint(const hitboxes::PosedHitbox &posed, const hitboxes::Definition &shape,
                                           TargetKey target, PointDistribution distribution,
                                           std::optional<std::uint64_t> seed)
{
    auto world = posed.center();
    if (seed)
    {
        const auto local = hitboxes::sampleInterior(shape, distribution, unitVariates(*seed, target, shape.id));
        if (!local)
            return {};
        world = posed.point(*local);
    }
    if (!pipeline::finite(world))
        return {};
    return AimCandidatePoint{world, shape.id, seed};
}
} // namespace

hitboxes::PointDistribution parseAimPointDistribution(std::string_view id)
{
    if (id == "uniform")
        return PointDistribution::Uniform;
    if (id == "center")
        return PointDistribution::Center;
    if (id == "outer")
        return PointDistribution::Outer;
    if (id == "middle")
        return PointDistribution::Middle;
    throw std::invalid_argument("CS2 Hitbox 瞄点分布模式无效");
}
HitboxAimSettings readHitboxAimSettings(const application::Json &values)
{
    HitboxAimSettings settings;
    settings.insetPercent = readHitboxInsetPercent(values);
    settings.intervalMs = values.value("cs2AimPointIntervalMs", settings.intervalMs);
    settings.distribution = parseAimPointDistribution(values.value("cs2AimPointDistribution", std::string("uniform")));
    return settings;
}
void HitboxAim::advance(std::uint64_t now, unsigned intervalMs)
{
    if (!seed_ || now < sampledAt_ || now - sampledAt_ >= intervalMs)
    {
        seed_ = random_();
        sampledAt_ = now;
    }
}
void HitboxAim::keepUntilNextInterval(std::uint64_t now) noexcept
{
    sampledAt_ = now;
}
void HitboxAim::reset() noexcept
{
    seed_.reset();
    visible_.reset();
}
std::optional<AimCandidatePoint> HitboxAim::point(const Snapshot &frame, const Body &body, TargetKey target,
                                                 unsigned part, const HitboxAimSettings &settings) const
{
    const auto posed = aimVolume(frame, body, part);
    if (!posed)
        return {};
    const auto shape = hitboxes::insetDefinition(*posed->definition, settings.insetPercent / 100);
    return drawPoint(*posed, shape, target, settings.distribution, seed_);
}

HitboxAim::Search HitboxAim::startSearch(bool locked)
{
    // Limit the work on each control tick. Every eligible person can take part in initial selection.
    return {random_(), locked ? 8u : 3u};
}

AimPointResolution HitboxAim::visiblePoint(const Snapshot &frame, const Body &body, TargetKey target, unsigned part,
                                          const HitboxAimSettings &settings, std::uint64_t now,
                                          std::optional<Search> search, bool preview, const AimPointProbe &probe) const
{
    const auto posed = aimVolume(frame, body, part);
    if (!posed)
        return {{}, "aim-hitbox-unavailable"};
    const auto shape = hitboxes::insetDefinition(*posed->definition, settings.insetPercent / 100);
    const auto draw = [&](std::optional<std::uint64_t> seed) {
        return drawPoint(*posed, shape, target, settings.distribution, seed);
    };
    std::optional<AimCandidatePoint> retained;
    std::string block;
    const bool sameHitbox = visible_ == TargetHitbox{target, shape.id};
    if (sameHitbox)
        retained = draw(seed_);
    else if (!search && preview)
        retained = draw({});
    if (retained)
    {
        block = probe(retained->position);
        const bool due = now < sampledAt_ || now - sampledAt_ >= settings.intervalMs;
        if (block == "ready" && (!search || !due))
            return {retained, {}};
        if (block != "ready")
            retained.reset();
        if (block == "cover-pending" || block == "cover-map-missing" || block == "cover-unavailable")
            return {{}, std::move(block)};
    }
    if (search)
    {
        auto seed = search->seed;
        for (unsigned attempt = 0; attempt < search->attempts; ++attempt)
        {
            seed = mix(seed);
            const auto candidate = draw(seed);
            if (!candidate)
                return {{}, "aim-hitbox-unavailable"};
            auto result = probe(candidate->position);
            if (result == "ready")
                return {candidate, {}};
            if (result == "cover-pending" || result == "cover-map-missing" || result == "cover-unavailable")
                return retained ? AimPointResolution{retained, {}} : AimPointResolution{{}, std::move(result)};
            if (!result.empty())
                block = std::move(result);
        }
    }
    if (retained)
        return {retained, {}};
    // Rejected rays cannot prove that every point is hidden. Another control tick can sample again.
    if (block == "cover-occluded" || (!search && !preview && !sameHitbox))
        block = "cover-searching";
    return {{}, std::move(block)};
}

void HitboxAim::keepVisible(TargetKey target, unsigned id, std::uint64_t seed, std::uint64_t now) noexcept
{
    const TargetHitbox key{target, id};
    if (visible_ != key || seed_ != seed)
    {
        visible_ = key;
        seed_ = seed;
        sampledAt_ = now;
    }
}
} // namespace mousebox::games::cs2
