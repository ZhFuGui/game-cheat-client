#pragma once
#include "application/json.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace mousebox::application {
inline constexpr std::size_t MaximumMouseReports=2000000;
struct MouseInputReport {
    std::uint64_t us=0;
    std::int32_t dx=0,dy=0;
    std::uint16_t flags=0,buttons=0;
};
struct MouseReportsProgress {
    bool recording=false;
    std::uint64_t durationUs=0;
    std::size_t reports=0,otherDeviceReports=0,absoluteReports=0;
    std::string device,reason="idle";
};
struct MouseReportRecording {
    MouseReportsProgress progress;
    std::vector<MouseInputReport> reports;
};
Json mouseReportsProgressJson(const MouseReportsProgress&);
Json analyzeMouseReports(const MouseReportRecording&);
bool saveMouseReportsCsv(const std::filesystem::path&,const MouseReportRecording&);
}
