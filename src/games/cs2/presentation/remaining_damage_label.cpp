#include "remaining_damage_label.hpp"
#include "games/cs2/extensions/frame_input.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>
namespace mousebox::games::cs2
{
std::wstring remainingDamageLabel(const extensions::RemainingDamageResult *result, const Snapshot &frame,
                                  const Player &player, bool english)
{
    if (!result || !player.body || result->scope != extensions::scope(frame) || result->capture != frame.sequence)
        return {};
    for (const auto &actor : result->actors)
        if (actor.controller == player.controller && actor.pawn == player.body->entity.id)
        {
            const auto prefix = english ? L"Bullet remaining " : L"弹头余伤 ";
            const auto &p = actor.prediction;
            if (p.outcome == application::ports::ShotOutcome::Stopped)
                return std::wstring(prefix) + (english ? L"Stopped" : L"未到达");
            if (p.outcome != application::ports::ShotOutcome::Reached || !p.remainingDamage ||
                !std::isfinite(*p.remainingDamage) || *p.remainingDamage < 0)
                return std::wstring(prefix) + L"—";
            std::wostringstream text;
            text << prefix << std::fixed << std::setprecision(1) << *p.remainingDamage;
            return text.str();
        }
    return {};
}
} // namespace mousebox::games::cs2
