#pragma once
#include "games/pubg/runtime/backend.hpp"
#include "application/json.hpp"
namespace mousebox::application {
Json defaultValues();
Json defaultEspValues();
void validateSetting(std::string_view key,const Json& value);
void validateEspSetting(std::string_view key,const Json& value);
Json normalizeSettings(const Json& source);
std::string calibrationSignature(const Json& values);
std::vector<control::CalibrationRecord> calibrationRecords(const Json&,const std::string& signature);
Json calibrationRecordsJson(const std::vector<control::CalibrationRecord>&);
void applySettings(Backend&,const Json& values);
}
