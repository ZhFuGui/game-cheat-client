#pragma once
#include "application/storage/json_store.hpp"
#include "games/pubg/model/control_frame.hpp"
#include "games/pubg/pipeline/ballistic_aim.hpp"
namespace mousebox::application {
// Raw observation DTO; active usage is reported separately from field presence.
Json describeReaderTelemetry(const std::optional<control::Frame>&,std::uint64_t nowMs,bool nativeControlActive=false);
Json describeNativeAimComparison(const std::optional<control::Frame>&,const control::BallisticDiagnostic&,std::uint64_t nowMs);
}
