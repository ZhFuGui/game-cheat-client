#pragma once
#include "control/human_motion.hpp"
#include "application/diagnostics/mouse_reports.hpp"
#include "raw_input_buffer.hpp"
#include "raw_registration.hpp"
#include <Windows.h>
#include <atomic>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#include <thread>
namespace mousebox::desktop
{
class RawCapture
{
    std::atomic<HWND> receiver_ = nullptr;
    HWND parent_ = nullptr;
    DWORD parentPid_ = 0;
    RECT bounds_{}, previousClip_{}, ownedClip_{};
    std::atomic<bool> active_ = false, closing_ = false;
    bool clipped_ = false, previousRestricted_ = false;
    RawRegistration registration_;
    RawInputBuffer inputBuffer_;
    std::uintptr_t device_ = 0;
    unsigned dpi_ = 1600;
    std::uint64_t startUs_ = 0;
    std::string session_;
    control::MouseTrial current_;
    std::vector<control::MouseTrial> finished_;
    std::size_t sampleCount_ = 0, trialCount_ = 0;
    bool reportsMode_ = false;
    application::MouseReportRecording reportRecording_;
    struct Call
    {
        std::function<void()> action;
        std::promise<void> done;
        std::atomic<bool> cancelled = false;
    };
    std::mutex callsMutex_;
    std::deque<std::shared_ptr<Call>> calls_;
    std::jthread thread_;
    std::atomic<DWORD> threadId_ = 0;
    void invoke(std::function<void()>);
    void loop(std::promise<void>);
    void drain();
    void unregister();
    void segment(bool next);
    void startImpl(unsigned, bool reports = false);
    void stopImpl(bool, std::string reason = "manual");
    void observeImpl();
    void inputImpl(HRAWINPUT);
    void acceptInput(const RAWINPUT &);
    static LRESULT CALLBACK procedure(HWND, UINT, WPARAM, LPARAM);

  public:
    std::function<void(bool, std::string, double, double)> onStatus;
    std::function<void(std::vector<control::MouseTrial>)> onTrials;
    std::function<void(application::MouseReportsProgress)> onReportStatus;
    std::function<void(application::MouseReportRecording)> onReports;
    RawCapture(HWND unusedReceiver, DWORD parentPid);
    ~RawCapture();
    void setBounds(HWND parent, RECT bounds);
    void start(unsigned dpi);
    void startReports();
    void stop(bool keep = true, std::string reason = "manual");
    void clear();
    // Kept for host compatibility. Capture scheduling belongs to its own thread.
    void observe();
    void input(HRAWINPUT);
    bool active() const
    {
        return active_.load();
    }
};
} // namespace mousebox::desktop
