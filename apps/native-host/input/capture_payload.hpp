#pragma once
#include "capture_channel.hpp"
#include "application/diagnostics/mouse_reports.hpp"
#include "control/human_motion.hpp"

namespace mousebox::desktop::capture
{
Frame encodeProgress(const application::MouseReportsProgress &);
application::MouseReportsProgress decodeProgress(const Json &);
Frame encodeReports(const application::MouseReportRecording &);
application::MouseReportRecording decodeReports(const Frame &);
Frame encodeTrials(const std::vector<control::MouseTrial> &);
std::vector<control::MouseTrial> decodeTrials(const Frame &);
} // namespace mousebox::desktop::capture
