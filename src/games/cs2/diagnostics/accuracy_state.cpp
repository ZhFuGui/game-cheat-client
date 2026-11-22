#include "accuracy_state.hpp"
#include "games/cs2/evaluation/weapon_accuracy.hpp"
#include "games/cs2/model/snapshot.hpp"
#include <array>

namespace mousebox::games::cs2
{
namespace
{
using application::Json;
Json unknown(const char *status, const char *reason)
{
    return {{"status", status}, {"value", nullptr}, {"reason", reason}};
}
Json measurement(const AccuracyScalar &result)
{
    if (result.malformed || result.status > 14 || ((result.status == 1) != result.value.has_value()))
        return unknown("inconsistent", "精度结果状态或数值不一致");
    if (result.status == 1)
        return {{"status", "valid"}, {"value", *result.value}, {"reason", ""}};
    constexpr std::array reasons = {"Reader 未提供结果",
                                    "",
                                    "读取失败",
                                    "采样过程中来源变化",
                                    "当前原生模型未核验",
                                    "原生函数分派未核验",
                                    "速度缓存等待原生重算",
                                    "物理速度分支尚未适配",
                                    "条件眼角来源不可用",
                                    "无法确认拥有者",
                                    "无法确认地面实体",
                                    "运行规则不可用",
                                    "原生输入无效",
                                    "原生求值结果非有限",
                                    "极端速度归一化尚未适配"};
    return unknown("unavailable", reasons[result.status]);
}
} // namespace
application::Json describeAccuracy(const Snapshot &frame, bool capability)
{
    const Weapon *weapon = nullptr;
    if (frame.directoryVerified && frame.local.controller && frame.local.pawn)
        for (const auto &player : frame.players)
            if (player.controller == *frame.local.controller && player.body &&
                player.body->entity.id == *frame.local.pawn && player.body->activeWeapon)
                for (const auto &candidate : frame.weapons)
                    if (candidate.entity.id == *player.body->activeWeapon)
                        weapon = &candidate;
    const auto accuracy = weapon && weapon->accuracy ? &*weapon->accuracy : nullptr;
    Json missing = unknown("unavailable", weapon ? "Reader 未提供武器精度观察" : "等待本地手持武器");
    Json inaccuracy = accuracy ? measurement(accuracy->inaccuracy) : missing;
    Json spread = accuracy ? measurement(accuracy->spread) : missing;
    const char *branch = "unavailable";
    Json components, interval;
    if (accuracy)
    {
        const auto &a = *accuracy;
        const auto invalidate = [&](const char *status, const char *reason) {
            inaccuracy = unknown(status, reason);
            spread = unknown(status, reason);
        };
        if (!capability)
            invalidate("unverified", "Reader 未声明武器精度能力");
        else
        {
            const auto issue = validateWeaponAccuracy(frame, a);
            if (issue == AccuracyIssue::None || issue == AccuracyIssue::InconsistentInaccuracy)
                interval = static_cast<double>(*a.readFinished - *a.started) * 1000.0 / frame.frequency;
            switch (issue)
            {
            case AccuracyIssue::Malformed:
                invalidate("inconsistent", "武器精度观察格式无效");
                break;
            case AccuracyIssue::UnverifiedModel:
                invalidate("unverified", "当前精度求值模型未核验");
                break;
            case AccuracyIssue::OutsideCapture:
                if (a.inaccuracy.value)
                    inaccuracy = unknown("inconsistent", "精度观察区间不属于本批读取");
                if (a.spread.value)
                    spread = unknown("inconsistent", "精度观察区间不属于本批读取");
                break;
            case AccuracyIssue::InconsistentInaccuracy:
                inaccuracy = unknown("inconsistent", "精度拥有者或求值分支不一致");
                if (a.owner == frame.local.pawn || a.branch == 1)
                    spread = unknown("inconsistent", "精度拥有者或求值分支不一致");
                break;
            case AccuracyIssue::None:
                if (a.inaccuracy.value)
                {
                    constexpr std::array branches = {"unavailable", "no-owner", "forced", "no-spread", "normal"};
                    branch = branches[a.branch];
                    if (a.branch == 4)
                        components = {{"penalty", *a.penalty},
                                      {"movement", *a.movement},
                                      {"airborne", *a.airborne},
                                      {"turning", *a.turning},
                                      {"lateral", *a.lateral}};
                }
                break;
            }
        }
    }
    return {{"readOnly", true},
            {"capture", std::to_string(frame.sequence)},
            {"weapon", weapon ? Json(std::to_string(weapon->entity.id)) : Json{}},
            {"owner", accuracy && accuracy->owner ? Json(std::to_string(*accuracy->owner)) : Json{}},
            {"modelId", accuracy ? Json(accuracy->modelId) : Json{}},
            {"branch", branch},
            {"inaccuracy", std::move(inaccuracy)},
            {"spread", std::move(spread)},
            {"components", std::move(components)},
            {"readIntervalMs", std::move(interval)}};
}
} // namespace mousebox::games::cs2
