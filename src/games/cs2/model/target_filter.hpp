#pragma once
namespace mousebox::games::cs2
{
struct TargetFilter
{
    bool enemies = true, teammates = false;
    bool any() const noexcept
    {
        return enemies || teammates;
    }
    bool allows(unsigned localTeam, unsigned targetTeam) const noexcept
    {
        return (localTeam == 2 || localTeam == 3) && (targetTeam == 2 || targetTeam == 3) &&
               (localTeam == targetTeam ? teammates : enemies);
    }
    bool operator==(const TargetFilter &) const = default;
};
} // namespace mousebox::games::cs2
