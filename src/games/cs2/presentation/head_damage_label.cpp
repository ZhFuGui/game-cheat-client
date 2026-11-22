#include "head_damage_label.hpp"
#include "games/cs2/extensions/frame_input.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>
namespace mousebox::games::cs2
{
std::wstring headDamageLabel(const extensions::HeadDamageResult *result, const Snapshot &frame, const Player &player,
                             bool english)
{
    if (!result || !player.body || result->scope != extensions::scope(frame) || result->capture != frame.sequence)
        return {};
    for (const auto &actor : result->actors)
        if (actor.controller == player.controller && actor.pawn == player.body->entity.id)
        {
            const auto prefix = english ? L"Head estimate " : L"预计头伤 ";
            const auto &p = actor.prediction;
            if (!p.healthDamage || !std::isfinite(*p.healthDamage) || *p.healthDamage < 0 || !p.healthLossMin ||
                !p.healthLossMax)
                return std::wstring(prefix) + L"—";
            if (p.flight.outcome == application::ports::ShotOutcome::Stopped)
                return std::wstring(prefix) + (english ? L"0 (stopped)" : L"0（未到达）");
            std::wostringstream out;
            out << prefix << L"≈" << std::fixed << std::setprecision(1) << *p.healthDamage
                << (english ? L" · HP " : L" · 扣血 ") << *p.healthLossMin;
            if (p.healthLossMin != p.healthLossMax)
                out << L"–" << *p.healthLossMax;
            return out.str();
        }
    return {};
}
} // namespace mousebox::games::cs2
