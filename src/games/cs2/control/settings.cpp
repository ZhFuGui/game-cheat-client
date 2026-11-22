#include "settings.hpp"
#include "application/settings/mouse_trajectory.hpp"
#include "modes.hpp"
#include <cmath>
#include <array>
#include <stdexcept>
namespace mousebox::games::cs2
{
std::optional<CountCalibration> countCalibration(const application::Json &v)
{
    if (v.is_null())
        return {};
    if (!v.is_object() || v.size() != 7 || v.value("version", 0) != 1 || !v.contains("build") ||
        !v.at("build").is_number_unsigned())
        throw std::invalid_argument("CS2计数校准记录格式无效");
    CountCalibration c;
    c.build = v.at("build").get<unsigned>();
    const auto number = [&](const char *key) {
        const auto &n = v.at(key);
        if (!n.is_number() || !std::isfinite(n.get<double>()))
            throw std::invalid_argument("CS2校准值须为有限数值");
        return n.get<double>();
    };
    c.x = number("x");
    c.y = number("y");
    c.yawReference = number("yawReference");
    c.pitchReference = number("pitchReference");
    c.responseUpperMs = number("responseUpperMs");
    if (v.at("build").get<std::uint64_t>() > 0xffffffffULL || std::abs(c.x) < .05 || std::abs(c.x) > 20 ||
        std::abs(c.y) < .05 || std::abs(c.y) > 20 || std::abs(c.yawReference) < 1e-7 ||
        std::abs(c.pitchReference) < 1e-7 || c.responseUpperMs < 0 || c.responseUpperMs > 500)
        throw std::invalid_argument("CS2校准值超出验证范围");
    return c;
}
application::Json calibrationJson(const CountCalibration &c)
{
    return {{"version", 1},
            {"build", c.build},
            {"x", c.x},
            {"y", c.y},
            {"yawReference", c.yawReference},
            {"pitchReference", c.pitchReference},
            {"responseUpperMs", c.responseUpperMs}};
}
namespace
{
constexpr std::array<std::string_view, 4> AlgorithmVersions{"v1", "v2", "v3", "v4"};
}
std::string_view algorithmVersionId(AimSettings::Version version)
{
    const auto i = static_cast<unsigned>(version);
    if (i >= AlgorithmVersions.size())
        throw std::invalid_argument("未知CS2瞄准算法版本");
    return AlgorithmVersions[i];
}
AimSettings::Version parseAlgorithmVersion(std::string_view id)
{
    for (unsigned i = 0; i < AlgorithmVersions.size(); ++i)
        if (AlgorithmVersions[i] == id)
            return static_cast<AimSettings::Version>(i);
    throw std::invalid_argument("未知CS2瞄准算法版本");
}
TargetFilter targetFilterSettings(const application::Json &values)
{
    // Keep the existing persisted keys when sharing the setting across Aim modes.
    return {values.value("cs2RageEnemies", true), values.value("cs2RageTeammates", false)};
}
AimSettings aimSettings(const application::Json &v)
{
    AimSettings s;
    const auto selected = parseAlgorithmMode(v.value("aimVersion", std::string("v1")));
    if (selected != ModeSelection{})
        throw std::invalid_argument("当前后座方案仅支持原生后坐求值");
    s.version = selected.version;
    s.comparison = selected.comparison;
    s.movement = control::movement::parseAlgorithm(v.value("mouseMovementAlgorithm", std::string("historical")));
    s.trajectory = application::readTrajectorySettings(v);
    s.inputMapping = v.at("cs2InputMapping") == "measured" ? AimSettings::InputMapping::Measured
                                                           : AimSettings::InputMapping::NativeFormula;
    s.part = v.at("cs2AimPart").get<unsigned>();
    s.targets = targetFilterSettings(v);
    s.weaponParts = {v.value("cs2AimPartPistol", 0), v.value("cs2AimPartRifle", 1), v.value("cs2AimPartSmg", 1),
                     v.value("cs2AimPartSniper", 2)};
    s.priority = v.at("cs2TargetPriority").get<unsigned>();
    s.coneDegrees = v.at("cs2AimCone").get<double>();
    s.maximumDistanceUnits = v.at("cs2AimDistanceUnits").get<double>();
    s.maximumAgeMs = v.at("cs2AimMaxAgeMs").get<double>();
    s.responseMs = v.at("cs2ResponseMs").get<double>();
    s.prediction = v.at("cs2Prediction").get<bool>();
    s.recoil = v.at("cs2RecoilCompensation").get<bool>();
    s.speed = static_cast<control::SpeedPreset>(v.at("speed").get<unsigned>());
    s.trigger = v.at("trigger").get<std::uint16_t>();
    s.calibration = countCalibration(v.at("cs2Calibration"));
    return s;
}
control::movement::Algorithm historicalMovementAlgorithm(AimSettings::Version version)
{
    using control::movement::Algorithm;
    if (version == AimSettings::Version::V2)
        return Algorithm::RecoilTracking;
    if (version == AimSettings::Version::V3)
        return Algorithm::Direct;
    return Algorithm::Smooth;
}
control::movement::Algorithm resolvedMovementAlgorithm(const AimSettings &settings)
{
    if (settings.comparison != ComparisonFactor::None)
        return control::movement::Algorithm::Smooth;
    return settings.movement == control::movement::Algorithm::Historical ? historicalMovementAlgorithm(settings.version)
                                                                         : settings.movement;
}
CountMapping resolveCountMapping(const AimSettings &s, unsigned /*build*/)
{
    if (s.inputMapping == AimSettings::InputMapping::NativeFormula)
        return {true, false, 1, 1};
    if (!s.calibration)
        return {};
    return {true, true, s.calibration->x, s.calibration->y};
}
} // namespace mousebox::games::cs2
