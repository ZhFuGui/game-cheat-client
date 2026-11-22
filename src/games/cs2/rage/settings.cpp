#include "settings.hpp"
#include <cmath>
#include <stdexcept>
namespace mousebox::games::cs2::rage
{
void addScoringDefaults(application::Json &v)
{
    const application::Json defaults{{"cs2RageScoreModel", "threat-v2"},
                                     {"cs2RageAimWeight", 60},
                                     {"cs2RageLowAmmoPenalty", 10},
                                     {"cs2RageReloadPenalty", 60},
                                     {"cs2RageHealthNormalization", true},
                                     {"cs2RageStableTarget", true},
                                     {"cs2RageSwitchMargin", 1}};
    for (const auto &[key, value] : defaults.items())
        v[key] = value;
}
bool permitsScoringSetting(std::string_view key)
{
    return key == "cs2RageScoreModel" || key == "cs2RageAimWeight" || key == "cs2RageLowAmmoPenalty" ||
           key == "cs2RageReloadPenalty" || key == "cs2RageHealthNormalization" || key == "cs2RageStableTarget" ||
           key == "cs2RageSwitchMargin";
}
bool validateScoringSetting(std::string_view key, const application::Json &v)
{
    if (!permitsScoringSetting(key))
        return false;
    bool valid;
    if (key == "cs2RageScoreModel")
        valid = v == "legacy" || v == "threat-v2";
    else if (key == "cs2RageHealthNormalization" || key == "cs2RageStableTarget")
        valid = v.is_boolean();
    else
        valid = v.is_number() && std::isfinite(v.get<double>()) && v.get<double>() >= 0 && v.get<double>() <= 100;
    if (!valid)
        throw std::invalid_argument("Rage评分设置无效：" + std::string(key));
    return true;
}
ScorePolicy scorePolicy(const application::Json &v)
{
    ScorePolicy p;
    p.model =
        v.value("cs2RageScoreModel", std::string("threat-v2")) == "legacy" ? ScoreModel::Legacy : ScoreModel::ThreatV2;
    p.threatWeight = v.value("cs2RageThreatWeight", 70.) / 100;
    p.aimWeight = v.value("cs2RageAimWeight", 60.) / 100;
    p.lowAmmoPenalty = v.value("cs2RageLowAmmoPenalty", 10.);
    p.reloadPenalty = v.value("cs2RageReloadPenalty", 60.);
    p.healthNormalization = v.value("cs2RageHealthNormalization", true);
    p.stabilize = v.value("cs2RageStableTarget", true);
    p.switchMargin = v.value("cs2RageSwitchMargin", 1.);
    return p;
}
} // namespace mousebox::games::cs2::rage
