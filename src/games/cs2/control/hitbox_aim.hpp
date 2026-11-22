#pragma once
#include "aim_types.hpp"
#include <random>

namespace mousebox::games::cs2
{
class HitboxAim
{
    struct TargetHitbox
    {
        TargetKey target;
        unsigned id;
        bool operator==(const TargetHitbox &) const = default;
    };
    std::mt19937_64 random_{std::random_device{}()};
    std::optional<std::uint64_t> seed_;
    std::optional<TargetHitbox> visible_;
    std::uint64_t sampledAt_ = 0;

  public:
    struct Search
    {
        std::uint64_t seed;
        unsigned attempts;
    };

    void advance(std::uint64_t now, unsigned intervalMs);
    void keepUntilNextInterval(std::uint64_t now) noexcept;
    void reset() noexcept;
    std::optional<AimCandidatePoint> point(const Snapshot &, const Body &, TargetKey, unsigned part,
                                         const HitboxAimSettings &) const;
    Search startSearch(bool locked);
    AimPointResolution visiblePoint(const Snapshot &, const Body &, TargetKey, unsigned part,
                                    const HitboxAimSettings &, std::uint64_t now, std::optional<Search>, bool preview,
                                    const AimPointProbe &) const;
    void keepVisible(TargetKey, unsigned id, std::uint64_t seed, std::uint64_t now) noexcept;
};
} // namespace mousebox::games::cs2
