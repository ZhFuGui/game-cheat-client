#include "settings.hpp"
#include "games/pubg/settings/application_adapter.hpp"

namespace mousebox::application
{
// Legacy PUBG API remains a thin compatibility facade. Workspace storage uses
// the explicit game configuration port instead of these default-game helpers.
Json defaultValues()
{
    return games::pubg::settings::defaultValues();
}
Json defaultEspValues()
{
    return games::pubg::settings::defaultEspValues();
}
void validateSetting(std::string_view key, const Json &value)
{
    games::pubg::settings::validateSetting(key, value);
}
void validateEspSetting(std::string_view key, const Json &value)
{
    games::pubg::settings::validateEspSetting(key, value);
}
Json normalizeSettings(const Json &source)
{
    return games::pubg::settings::normalizeSettings(source);
}
std::string calibrationSignature(const Json &values)
{
    return games::pubg::settings::calibrationSignature(values);
}
std::vector<control::CalibrationRecord> calibrationRecords(const Json &source, const std::string &signature)
{
    return games::pubg::settings::calibrationRecords(source, signature);
}
Json calibrationRecordsJson(const std::vector<control::CalibrationRecord> &records)
{
    return games::pubg::settings::calibrationRecordsJson(records);
}
void applySettings(Backend &backend, const Json &values)
{
    games::pubg::settings::applySettings(backend, values);
}
} // namespace mousebox::application
