#pragma once
#include "capture_channel.hpp"
#include "application/diagnostics/mouse_reports.hpp"
#include "control/human_motion.hpp"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace mousebox::desktop
{
class CaptureClient
{
    DWORD parentPid_;
    std::string token_;
    capture::Handle cancel_, commands_, events_;
    std::jthread reader_;
    std::mutex commandMutex_, eventMutex_;
    std::condition_variable delivered_;
    std::uint64_t sequence_ = 0;
    std::atomic<bool> failed_ = false, closing_ = false, connected_ = false;
    capture::Json call(capture::Json);
    void receive();
    void accept(const capture::Frame &);

  public:
    std::function<void(bool, std::string, double, double)> onStatus;
    std::function<void(std::vector<control::MouseTrial>)> onTrials;
    std::function<void(application::MouseReportsProgress)> onReportStatus;
    std::function<void(application::MouseReportRecording)> onReports;
    CaptureClient(DWORD parentPid, std::string token);
    ~CaptureClient();
    void connect();
    void setBounds(HWND, RECT);
    void start(unsigned);
    void startReports();
    void stop(bool keep = true, std::string reason = "manual");
    void clear();
    capture::Json inspect();
};
} // namespace mousebox::desktop
