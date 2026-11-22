#pragma once
#include "control/human_motion.hpp"
#include <filesystem>
namespace mousebox::application {
enum class ModelFileKind {Unknown,LegacyPointing,Speed};
bool saveMouseTrials(const std::filesystem::path&,const std::vector<control::MouseTrial>&);
std::vector<control::MouseTrial> loadMouseTrials(const std::filesystem::path&);
std::vector<control::MouseTrial> loadMouseTrialFiles(const std::vector<std::filesystem::path>&);
bool saveHumanModel(const std::filesystem::path&,const control::HumanMotionModel&);
control::HumanMotionModel loadHumanModel(const std::filesystem::path&);
ModelFileKind humanModelFileKind(const std::filesystem::path&);
bool removeHumanModel(const std::filesystem::path&);
}
