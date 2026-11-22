#include "configuration.hpp"
#include "hitbox_inset.hpp"
#include "games/cs2/trigger/click_timing.hpp"
#include "games/cs2/trigger/weapon_filter.hpp"
#include "application/settings/application_values.hpp"
#include "application/settings/random_range.hpp"
#include "application/settings/mouse_trajectory.hpp"
#include "games/cs2/control/settings.hpp"
#include "games/cs2/control/modes.hpp"
#include "games/cs2/extensions/registry.hpp"
#include "games/cs2/rage/settings.hpp"
#include <cmath>
#include <stdexcept>
namespace mousebox::games::cs2
{
using application::Json;
namespace
{
void require(bool b, const char *m)
{
    if (!b)
        throw std::invalid_argument(m);
}
void number(const Json &v, double lo, double hi, bool integer = false)
{
    require(v.is_number(), "CS2参数须为数值");
    auto n = v.get<double>();
    require(std::isfinite(n) && n >= lo && n <= hi && (!integer || n == std::floor(n)), "CS2参数超出范围");
}
bool retiredRecoilSetting(std::string_view key)
{
    return key == "cs2RcsEnabled" || key == "cs2RcsBinding" || key == "cs2RcsSpeed" ||
           key == "cs2RcsMaxAgeMs" || key == "cs2RcsResponseMs";
}

struct LegacyInsetSetting
{
    const char *value, *enabled;
};
constexpr LegacyInsetSetting LegacyInsetSettings[]{{"cs2AimInsetPercent", "aimEnabled"},
                                                   {"cs2TriggerInsetPercent", "cs2TriggerEnabled"},
                                                   {"cs2RageInsetPercent", "cs2RageEnabled"}};
const LegacyInsetSetting *legacyInsetSetting(std::string_view key)
{
    for (const auto &setting : LegacyInsetSettings)
        if (setting.value == key)
            return &setting;
    return nullptr;
}

void migrateHitboxInset(const Json &oldValues, Json &values)
{
    if (oldValues.contains(HitboxInsetSetting))
        return;
    const Json *fallback = nullptr;
    for (const auto &[key, enabled] : LegacyInsetSettings)
        if (const auto saved = oldValues.find(key); saved != oldValues.end())
        {
            if (values.at(enabled) == true)
            {
                values[HitboxInsetSetting] = *saved;
                return;
            }
            if (!fallback)
                fallback = &*saved;
        }
    if (fallback)
        values[HitboxInsetSetting] = *fallback;
}

class Configuration final : public application::ports::GameConfiguration
{
  public:
    std::string_view gameId() const noexcept override
    {
        return "cs2";
    }
    Json defaults() const override
    {
        auto out = Json{{"schemaVersion", 1},
                        {"speedSchema", 2},
                        {"profiles", Json::array()},
                        {"values",
                         {{"host", "127.0.0.1"},
                          {"port", 13202},
                          {"hz", 30},
                          {"dpi", 1600},
                          {"renderAdapter", "auto"},
                          {"cs2MetersPerUnit", 0.0},
                          {"cs2MaxAgeMs", 1000},
                          {"cs2HitboxDisplay", "off"},
                          {"cs2ShowHealth", true},
                          {"aimEnabled", true},
                          {"cs2CheckForeground", true},
                          {"cs2AimPart", 0},
                          {"cs2AimPartPistol", 0},
                          {"cs2AimPartRifle", 1},
                          {"cs2AimPartSmg", 1},
                          {"cs2AimPartSniper", 2},
                          {HitboxInsetSetting, DefaultHitboxInsetPercent},
                          {"cs2AimPointIntervalMs", HitboxAimSettings{}.intervalMs},
                          {"cs2AimPointDistribution", "uniform"},
                          {"cs2AimCoverCheck", false},
                          {"cs2TargetPriority", 0},
                          {"cs2AimCone", 15.0},
                          {"cs2AimDistanceUnits", 0.0},
                          {"cs2AimMaxAgeMs", 150},
                          {"cs2ResponseMs", 20},
                          {"cs2Prediction", true},
                          {"cs2RecoilCompensation", true},
                          {"speed", 2},
                          {"trigger", 8},
                          {"cs2Calibration", nullptr},
                          {"cs2InputMapping", "native"},
                          {"aimVersion", "v1"},
                          {"mouseMovementAlgorithm", "smooth"},
                          {"cs2TriggerEnabled", false},
                          {"cs2TriggerSnipers", true},
                          {"cs2TriggerSniperInaccuracyLimit", trigger::DefaultSniperInaccuracyLimit},
                          {"cs2TriggerOtherGuns", true},
                          {"cs2TriggerHoldRangeMs", application::randomRangeJson(trigger::DefaultHold)},
                          {"cs2TriggerTimeOffsetMs", 0},
                          {"cs2TriggerCoverCheck", false},
                          {"cs2TriggerReactionEnabled", false},
                          {"cs2TriggerReactionMs", 0},
                          {"cs2TriggerIntervalRangeMs", application::randomRangeJson(trigger::DefaultInterval)},
                          {"cs2TriggerMaxAgeMs", 150},
                          {"cs2RageEnabled", false},
                          {"cs2RageEnemies", true},
                          {"cs2RageTeammates", false},
                          {"cs2RageCoverCheck", true},
                          {"cs2RageOtherPlayerCheck", true},
                          {"cs2RageThreatWeight", 70},
                          {"cs2RageHoldMs", 20},
                          {"cs2RageIntervalMs", 100}}},
                        {"espValues",
                         {{"marker", "point"},
                          {"names", true},
                          {"distance", false},
                          {"bones", true},
                          {"tracers", false},
                          {"crosshair", true},
                          {"aimRange", false},
                          {"targetHighlight", false},
                          {"teammates", true},
                          {"opponents", true},
                          {"bots", true},
                          {"self", false},
                          {"aspect", "16:9"},
                          {"fovAxis", "horizontal"},
                          {"estimatedHeightMeters", 1.0},
                          {"maximumDistanceMeters", 0.0}}}};
        extensions::addSettingDefaults(out["values"]);
        application::addTrajectoryDefaults(out["values"]);
        rage::addScoringDefaults(out["values"]);
        return out;
    }
    Json normalize(const Json &source) const override
    {
        auto out = defaults();
        for (auto name : {"values", "espValues"})
        {
            auto object = source.value(name, Json::object());
            require(object.is_object(), "CS2设置分组须为对象");
            for (auto it = object.begin(); it != object.end(); ++it)
            {
                if (std::string_view(name) == "values" && retiredRecoilSetting(it.key()))
                    continue;
                if (std::string_view(name) == "values" &&
                    (it.key() == "cs2RageMaxAgeMs" || it.key() == "cs2TriggerIntervalMs" ||
                     it.key() == "cs2TriggerHoldMs" || it.key() == "aimVersion" || legacyInsetSetting(it.key())))
                {
                    validateStoredValue(it.key(), it.value());
                    continue;
                }
                if (std::string_view(name) == "values" && it.key() == "mouseMovementAlgorithm")
                {
                    validateStoredValue(it.key(), it.value());
                    out[name][it.key()] = it.value();
                    continue;
                }
                if (std::string_view(name) == "values")
                    validateValue(it.key(), it.value());
                else
                    validateEspValue(it.key(), it.value());
                out[name][it.key()] = it.value();
            }
        }
        if (source.contains("profiles"))
        {
            require(source["profiles"].is_array() && source["profiles"].empty(), "CS2不接受其他游戏的校准记录");
            out["profiles"] = source["profiles"];
        }
        const auto oldValues = source.value("values", Json::object());
        if (out["values"]["mouseMovementAlgorithm"] == "historical" || !oldValues.contains("mouseMovementAlgorithm"))
        {
            const auto legacy = parseAlgorithmMode(oldValues.value("aimVersion", std::string("v1")));
            out["values"]["mouseMovementAlgorithm"] =
                control::movement::algorithmId(historicalMovementAlgorithm(legacy.version));
        }
        const auto migrateTiming = [&](const char *legacy, const char *key, unsigned oldDefault) {
            if (oldValues.contains(key) || !oldValues.contains(legacy))
                return;
            const auto value = oldValues.at(legacy).get<unsigned>();
            if (value != oldDefault)
                out["values"][key] = application::randomRangeJson({value, value});
        };
        migrateTiming("cs2TriggerIntervalMs", "cs2TriggerIntervalRangeMs", 100);
        migrateTiming("cs2TriggerHoldMs", "cs2TriggerHoldRangeMs", 20);
        for (const auto key : {"cs2TriggerIntervalRangeMs", "cs2TriggerHoldRangeMs"})
        {
            auto &range = out["values"][key];
            if (!range.contains("distribution"))
                range["distribution"] = "normal";
        }
        if (!oldValues.contains("cs2InputMapping") && !out["values"]["cs2Calibration"].is_null())
            out["values"]["cs2InputMapping"] = "measured";
        if (out["values"]["cs2RageEnabled"] == true)
        {
            out["values"]["aimEnabled"] = false;
            out["values"]["cs2TriggerEnabled"] = false;
        }
        else if (out["values"]["cs2TriggerEnabled"] == true)
            out["values"]["aimEnabled"] = false;
        migrateHitboxInset(oldValues, out["values"]);
        return out;
    }
    Json updatedValues(const Json &current, std::string_view key, const Json &value) const override
    {
        auto next = GameConfiguration::updatedValues(current, key, value);
        if (value == true && (key == "aimEnabled" || key == "cs2TriggerEnabled" || key == "cs2RageEnabled"))
            for (const auto name : {"aimEnabled", "cs2TriggerEnabled", "cs2RageEnabled"})
                if (key != name)
                    next[name] = false;
        return next;
    }
    bool permitsAbsentValue(std::string_view key) const noexcept override
    {
        if (application::isTrajectorySetting(key))
            return true;
        if (rage::permitsScoringSetting(key))
            return true;
        return key == "aimEnabled" || key == "cs2CheckForeground" || key == "cs2AimCoverCheck" ||
               key == "cs2AimPartPistol" || key == "cs2AimPartRifle" || key == "cs2AimPartSmg" ||
               key == "cs2AimPartSniper" || key == HitboxInsetSetting || key == "cs2AimPointIntervalMs" ||
               key == "cs2AimPointDistribution" || key == "cs2RageEnabled" || key == "cs2RageEnemies" ||
               key == "cs2RageTeammates" || key == "cs2RageCoverCheck" || key == "cs2RageOtherPlayerCheck" ||
               key == "cs2RageThreatWeight" || key == "cs2RageHoldMs" ||
               key == "cs2RageIntervalMs" || key == "cs2TriggerSnipers" || key == "cs2TriggerOtherGuns" ||
               key == "cs2TriggerHoldRangeMs" || key == "cs2TriggerCoverCheck" ||
               key == "cs2TriggerReactionEnabled" || key == "cs2TriggerReactionMs" || key == "cs2TriggerEnabled" ||
               key == "cs2TriggerIntervalRangeMs" || key == "cs2TriggerMaxAgeMs" || extensions::permitsSetting(key) ||
               key == "cs2HitboxDisplay" || key == "cs2ShowHealth" || key == "cs2AimPart" ||
               key == "cs2TargetPriority" || key == "cs2AimCone" || key == "cs2AimDistanceUnits" ||
               key == "cs2AimMaxAgeMs" || key == "cs2ResponseMs" || key == "cs2Prediction" ||
               key == "cs2RecoilCompensation" || key == "speed" || key == "trigger" || key == "cs2Calibration" ||
               key == "cs2InputMapping" || key == "aimVersion" || key == "mouseMovementAlgorithm" ||
               key == "cs2TriggerTimeOffsetMs" || key == "cs2TriggerSniperInaccuracyLimit";
    }
    void validateStoredValue(std::string_view key, const Json &value) const override
    {
        if (retiredRecoilSetting(key))
            return;
        if (key == "cs2RageMaxAgeMs")
            number(value, 20, 500, true);
        else if (legacyInsetSetting(key))
            number(value, 0, 90);
        else if (key == "cs2TriggerIntervalMs")
            number(value, trigger::IntervalBounds.min, trigger::IntervalBounds.max, true);
        else if (key == "cs2TriggerHoldMs")
            number(value, trigger::HoldBounds.min, trigger::HoldBounds.max, true);
        else if (key == "aimVersion")
        {
            require(value.is_string(), "后座方案须为稳定ID");
            parseAlgorithmMode(value.get_ref<const std::string &>());
        }
        else if (key == "mouseMovementAlgorithm")
        {
            require(value.is_string(), "速度变化控制须为稳定ID");
            control::movement::parseAlgorithm(value.get_ref<const std::string &>());
        }
        else
            validateValue(key, value);
    }
    void validateValue(std::string_view key, const Json &v) const override
    {
        if (application::isTrajectorySetting(key))
        {
            application::validateTrajectorySetting(key, v);
            return;
        }
        if (rage::validateScoringSetting(key, v))
            return;
        if (extensions::validateSetting(key, v))
            return;
        if (key == "dpi" || key == "renderAdapter")
        {
            application::validateApplicationValue(key, v);
            return;
        }
        if (key == "host")
            require(v.is_string() && !v.get_ref<const std::string &>().empty() &&
                        v.get_ref<const std::string &>().size() <= 255 &&
                        v.get_ref<const std::string &>().find_first_not_of(" \t\r\n") != std::string::npos,
                    "Reader地址须为1–255字符的非空主机地址");
        else if (key == "port")
            number(v, 1, 65535, true);
        else if (key == "hz")
            number(v, 1, 300, true);
        else if (key == "cs2MetersPerUnit")
            number(v, 0, 1000);
        else if (key == "cs2MaxAgeMs")
            number(v, 50, 5000, true);
        else if (key == "cs2HitboxDisplay")
            require(v == "off" || v == "solid" || v == "wireframe", "CS2命中体显示模式无效");
        else if (key == "cs2AimPart" || key == "speed")
            number(v, 0, 4, true);
        else if (key == "cs2AimPartPistol" || key == "cs2AimPartRifle" || key == "cs2AimPartSmg" ||
                 key == "cs2AimPartSniper")
            number(v, -1, 4, true);
        else if (key == "cs2TargetPriority")
            number(v, 0, 1, true);
        else if (key == "cs2AimCone")
            number(v, .1, 90);
        else if (key == "cs2AimDistanceUnits")
            number(v, 0, 1000000);
        else if (key == "cs2RageThreatWeight")
            number(v, 0, 100);
        else if (key == HitboxInsetSetting)
            number(v, 0, 90);
        else if (key == "cs2AimPointIntervalMs")
            number(v, 1, 5000, true);
        else if (key == "cs2AimPointDistribution")
        {
            require(v.is_string(), "CS2 Hitbox 瞄点分布须为稳定 ID");
            parseAimPointDistribution(v.get_ref<const std::string &>());
        }
        else if (key == "cs2TriggerIntervalRangeMs")
            application::readRandomRange(v, trigger::IntervalBounds.min, trigger::IntervalBounds.max);
        else if (key == "cs2TriggerHoldRangeMs")
            application::readRandomRange(v, trigger::HoldBounds.min, trigger::HoldBounds.max);
        else if (key == "cs2RageIntervalMs")
            number(v, 50, 1000, true);
        else if (key == "cs2RageHoldMs")
            number(v, 5, 40, true);
        else if (key == "cs2TriggerReactionMs")
            number(v, 0, 5000, true);
        else if (key == "cs2AimMaxAgeMs" || key == "cs2TriggerMaxAgeMs")
            number(v, 20, 500, true);
        else if (key == "cs2ResponseMs")
            number(v, 0, 150, true);
        else if (key == "cs2TriggerTimeOffsetMs")
            number(v, -100, 100, true);
        else if (key == "cs2TriggerSniperInaccuracyLimit")
            number(v, 0, 1);
        else if (key == "aimEnabled" || key == "cs2CheckForeground" || key == "cs2AimCoverCheck" ||
                 key == "cs2TriggerCoverCheck" || key == "cs2TriggerReactionEnabled" || key == "cs2Prediction" ||
                 key == "cs2RecoilCompensation" || key == "cs2ShowHealth" ||
                 key == "cs2TriggerEnabled" || key == "cs2TriggerSnipers" || key == "cs2TriggerOtherGuns" ||
                 key == "cs2RageEnabled" || key == "cs2RageEnemies" || key == "cs2RageTeammates" ||
                 key == "cs2RageCoverCheck" || key == "cs2RageOtherPlayerCheck")
            require(v.is_boolean(), "CS2开关须为布尔值");
        else if (key == "trigger")
            require(v == 8 || v == 16, "CS2触发键须为鼠标侧键");
        else if (key == "cs2Calibration")
            countCalibration(v);
        else if (key == "cs2InputMapping")
            require(v == "native" || v == "measured", "CS2输入换算模式无效");
        else if (key == "mouseMovementAlgorithm")
        {
            require(v == "smooth" || v == "recoil-tracking" || v == "direct", "速度变化控制模式无效");
        }
        else if (key == "aimVersion")
        {
            require(v == "v1", "当前后座方案仅支持原生后坐求值");
        }
        else
            throw std::invalid_argument("未知CS2设置：" + std::string(key) + "；不接受其他游戏的参数");
    }
    void validateEspValue(std::string_view key, const Json &v) const override
    {
        if (key == "marker")
            require(v == "point" || v == "corners", "CS2仅提供定位点或原始骨点范围角标");
        else if (key == "aspect")
            require(v == "16:9" || v == "16:10" || v == "21:9" || v == "4:3" || v == "viewport", "CS2视口比例无效");
        else if (key == "maximumDistanceMeters")
            number(v, 0, 100000);
        else if (key == "fovAxis")
            require(v == "horizontal", "CS2使用Reader矩阵，不能另行解释FOV");
        else if (key == "estimatedHeightMeters")
            require(v == 1.0, "CS2不使用人体高度估算");
        else if (key == "aimRange" || key == "targetHighlight")
            require(v == false, "CS2尚未开放瞄准功能");
        else
        {
            auto defaultsEsp = defaults()["espValues"];
            require(defaultsEsp.contains(key) && defaultsEsp[std::string(key)].is_boolean() && v.is_boolean(),
                    "未知CS2显示设置");
        }
    }
};
} // namespace
const application::ports::GameConfiguration &configuration()
{
    static const Configuration c;
    return c;
}
} // namespace mousebox::games::cs2
