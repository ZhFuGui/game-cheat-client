#pragma once
#include "games/pubg/runtime/runtime_models.hpp"
#include "application/json.hpp"
namespace mousebox::application {
struct AimAlgorithmReport {Json sections,values;};
AimAlgorithmReport describeAimAlgorithm(const Snapshot&,const control::LiveDiagnostic&,std::uint64_t);
}
