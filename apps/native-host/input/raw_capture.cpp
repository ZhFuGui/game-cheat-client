#include "raw_capture.hpp"
#include "platform/windows/input.hpp"
#include "application/storage/json_store.hpp"
#include <algorithm>
#include <chrono>
#include <stdexcept>
namespace mousebox::desktop
{
namespace
{
std::uint64_t nowUs()
{
    using namespace std::chrono;
    return static_cast<std::uint64_t>(duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
}
constexpr UINT dispatchMessage = WM_APP + 19, cancelMessage = WM_APP + 20;
} // namespace
RawCapture::RawCapture(HWND, DWORD parentPid) : parentPid_(parentPid)
{
    std::promise<void> ready;
    auto result = ready.get_future();
    thread_ = std::jthread([this, ready = std::move(ready)]() mutable { loop(std::move(ready)); });
    result.get();
}
RawCapture::~RawCapture()
{
    try
    {
        stop();
    }
    catch (const std::exception &)
    {
    }
    closing_ = true;
    if (receiver_)
        PostMessageW(receiver_, WM_CLOSE, 0, 0);
    thread_.request_stop();
    if (thread_.joinable())
        thread_.join();
}
void RawCapture::invoke(std::function<void()> action)
{
    if (GetCurrentThreadId() == threadId_.load())
    {
        action();
        return;
    }
    if (closing_)
        throw std::runtime_error("原始输入线程已停止");
    auto call = std::make_shared<Call>();
    call->action = std::move(action);
    auto result = call->done.get_future();
    {
        const std::lock_guard lock(callsMutex_);
        if (closing_)
            throw std::runtime_error("原始输入线程已停止");
        if (calls_.size() >= 64)
            throw std::runtime_error("原始输入命令队列已满");
        calls_.push_back(call);
    }
    if (!PostMessageW(receiver_, dispatchMessage, 0, 0))
    {
        const std::lock_guard lock(callsMutex_);
        const auto i = std::find(calls_.begin(), calls_.end(), call);
        if (i != calls_.end())
        {
            calls_.erase(i);
            call->done.set_exception(std::make_exception_ptr(std::runtime_error("原始输入线程不可用")));
        }
    }
    if (result.wait_for(std::chrono::seconds(2)) != std::future_status::ready)
    {
        call->cancelled = true;
        PostMessageW(receiver_, cancelMessage, 0, 0);
        throw std::runtime_error("原始输入命令超时，采集请求已撤销");
    }
    result.get();
}
void RawCapture::drain()
{
    std::deque<std::shared_ptr<Call>> calls;
    {
        const std::lock_guard lock(callsMutex_);
        calls.swap(calls_);
    }
    for (const auto &call : calls)
    {
        try
        {
            if (call->cancelled)
                throw std::runtime_error("原始输入命令已撤销");
            call->action();
            if (call->cancelled)
            {
                stopImpl(false);
                throw std::runtime_error("原始输入命令已撤销");
            }
            call->done.set_value();
        }
        catch (...)
        {
            call->done.set_exception(std::current_exception());
        }
    }
}
void RawCapture::loop(std::promise<void> ready)
{
    threadId_ = GetCurrentThreadId();
    try
    {
        const WNDCLASSW cls{0,       procedure, 0,       0,       GetModuleHandleW(nullptr),
                            nullptr, nullptr,   nullptr, nullptr, L"MouseboxRawCaptureMessages"};
        if (!RegisterClassW(&cls) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            throw std::runtime_error("原始输入消息类创建失败");
        receiver_ =
            CreateWindowExW(0, cls.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, cls.hInstance, this);
        if (!receiver_)
            throw std::runtime_error("原始输入消息窗口创建失败");
        ready.set_value();
    }
    catch (...)
    {
        if (receiver_)
            DestroyWindow(receiver_);
        receiver_ = nullptr;
        closing_ = true;
        ready.set_exception(std::current_exception());
        return;
    }
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    try
    {
        stopImpl(true);
    }
    catch (...)
    {
        unregister();
    }
    closing_ = true;
    {
        const std::lock_guard lock(callsMutex_);
        for (const auto &call : calls_)
            call->done.set_exception(std::make_exception_ptr(std::runtime_error("原始输入线程已停止")));
        calls_.clear();
    }
    KillTimer(receiver_, 1);
    DestroyWindow(receiver_);
    receiver_ = nullptr;
}
LRESULT CALLBACK RawCapture::procedure(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    auto capture = reinterpret_cast<RawCapture *>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        capture = static_cast<RawCapture *>(reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(capture));
    }
    if (capture)
    {
        try
        {
            if (message == dispatchMessage)
            {
                capture->drain();
                return 0;
            }
            if (message == cancelMessage)
            {
                capture->stopImpl(false);
                return 0;
            }
            if (message == WM_TIMER)
            {
                capture->observeImpl();
                return 0;
            }
            if (message == WM_INPUT)
            {
                capture->inputImpl(reinterpret_cast<HRAWINPUT>(lp));
                return DefWindowProcW(window, message, wp, lp);
            }
            if (message == WM_CLOSE)
            {
                try
                {
                    capture->stopImpl(true);
                }
                catch (...)
                {
                    capture->active_ = false;
                    capture->unregister();
                }
                PostQuitMessage(0);
                return 0;
            }
        }
        catch (const std::exception &error)
        {
            try
            {
                if (capture->reportsMode_)
                    capture->stopImpl(true, "error");
                else
                {
                    capture->active_ = false;
                    capture->unregister();
                    if (capture->onStatus)
                        capture->onStatus(false, error.what(), 0, 0);
                }
            }
            catch (...)
            {
                capture->active_ = false;
                capture->unregister();
            }
            if (message == WM_INPUT)
                return DefWindowProcW(window, message, wp, lp);
            return 0;
        }
    }
    return DefWindowProcW(window, message, wp, lp);
}
void RawCapture::unregister()
{
    KillTimer(receiver_, 1);
    registration_.release();
    if (clipped_)
    {
        RECT current{};
        if (GetClipCursor(&current) && EqualRect(&current, &ownedClip_))
            ClipCursor(previousRestricted_ ? &previousClip_ : nullptr);
        clipped_ = false;
    }
}
void RawCapture::setBounds(HWND parent, RECT bounds)
{
    invoke([this, parent, bounds] {
        if (active_ && (parent_ != parent || !EqualRect(&bounds_, &bounds)))
            stopImpl(true, "bounds");
        parent_ = parent;
        bounds_ = bounds;
    });
}
void RawCapture::start(unsigned dpi)
{
    invoke([this, dpi] { startImpl(dpi); });
}
void RawCapture::startReports()
{
    invoke([this] { startImpl(1600, true); });
}
void RawCapture::stop(bool keep, std::string reason)
{
    invoke([this, keep, reason = std::move(reason)] { stopImpl(keep, reason); });
}
void RawCapture::clear()
{
    invoke([this] {
        stopImpl(false);
        finished_.clear();
        sampleCount_ = 0;
        trialCount_ = 0;
    });
}
void RawCapture::observe() {}
void RawCapture::input(HRAWINPUT handle)
{
    if (GetCurrentThreadId() == threadId_.load())
        inputImpl(handle);
}
void RawCapture::startImpl(unsigned dpi, bool reports)
{
    stopImpl(true);
    DWORD owner = 0;
    GetWindowThreadProcessId(parent_, &owner);
    if (!IsWindow(parent_) || owner != parentPid_ || GetForegroundWindow() != parent_ || IsIconic(parent_) ||
        bounds_.right <= bounds_.left || bounds_.bottom <= bounds_.top || dpi < 100 || dpi > 32000)
        throw std::invalid_argument("请将鼠标活动区域完整显示在前台后开始采集");
    if (parentPid_ != GetCurrentProcessId())
        throw std::runtime_error("采集必须位于拥有前台窗口的进程中");
    if (reports)
    {
        reportRecording_ = {};
        reportRecording_.reports.reserve(application::MaximumMouseReports);
    }
    // Prepare before subscribing, and discard the previous session's queued
    // records. Never reset the clock after new input can already be queued.
    inputBuffer_.drain([](const RAWINPUT &) {});
    startUs_ = nowUs();
    registration_.acquire(receiver_);
    try
    {
        if (!SetTimer(receiver_, 1, 16, nullptr))
            throw std::runtime_error("原始输入资格计时器创建失败");
        if (!GetClipCursor(&previousClip_))
            throw std::runtime_error("无法读取光标范围");
        previousRestricted_ = previousClip_.left != GetSystemMetrics(SM_XVIRTUALSCREEN) ||
                              previousClip_.top != GetSystemMetrics(SM_YVIRTUALSCREEN) ||
                              previousClip_.right - previousClip_.left != GetSystemMetrics(SM_CXVIRTUALSCREEN) ||
                              previousClip_.bottom - previousClip_.top != GetSystemMetrics(SM_CYVIRTUALSCREEN);
        POINT p{bounds_.left, bounds_.top};
        ClientToScreen(parent_, &p);
        RECT clip{p.x, p.y, p.x + bounds_.right - bounds_.left, p.y + bounds_.bottom - bounds_.top};
        if (!ClipCursor(&clip))
            throw std::runtime_error("无法限制采集光标范围");
        ownedClip_ = clip;
        clipped_ = true;
        reportsMode_ = reports;
        dpi_ = dpi;
        device_ = 0;
        finished_.clear();
        sampleCount_ = 0;
        trialCount_ = 0;
        if (reportsMode_)
        {
            reportRecording_.progress.recording = true;
            reportRecording_.progress.reason = "recording";
            active_ = true;
            if (onReportStatus)
                onReportStatus(reportRecording_.progress);
        }
        else
        {
            session_ = std::to_string(nowUs()) + "-" + std::to_string(randomSession());
            current_ = {dpi_, {{0, 0, 0}}, session_};
            active_ = true;
            if (onStatus)
                onStatus(true, "自然移动、起步和停止；每6秒保存一段，按 Esc 暂停。", 0, 0);
        }
    }
    catch (...)
    {
        active_ = false;
        unregister();
        throw;
    }
}
void RawCapture::segment(bool next)
{
    if (current_.samples.empty())
        return;
    const auto us = nowUs() - startUs_;
    const auto last = current_.samples.back();
    if (us > last.us && us <= 15000000 && sampleCount_ + current_.samples.size() < 2000000)
        current_.samples.push_back({us, last.x, last.y});
    if (us >= 800000 && us <= 15000000 && current_.samples.size() >= 20)
    {
        sampleCount_ += current_.samples.size();
        ++trialCount_;
        finished_.push_back(std::move(current_));
    }
    current_ = {};
    if (next)
    {
        current_ = {dpi_, {{0, 0, 0}}, session_};
        startUs_ = nowUs();
    }
}
void RawCapture::stopImpl(bool keep, std::string reason)
{
    const bool was = active_.exchange(false);
    unregister();
    if (was && reportsMode_)
    {
        auto &progress = reportRecording_.progress;
        progress.recording = false;
        progress.durationUs = nowUs() - startUs_;
        progress.reports = reportRecording_.reports.size();
        progress.reason = std::move(reason);
        // Publish the finished state and its recording together. A separate
        // inactive notification would allow a new capture before analysis starts.
        if (keep && onReports)
            onReports(std::move(reportRecording_));
        else if (onReportStatus)
            onReportStatus(progress);
        reportRecording_ = {};
        reportsMode_ = false;
        return;
    }
    if (was && keep)
        segment(false);
    current_ = {};
    if (was)
    {
        if (onTrials && !finished_.empty())
            onTrials(std::move(finished_));
        finished_.clear();
        if (onStatus)
            onStatus(false, "已暂停并保存有效移动；再次开始属于新的采集场次。", 0, 0);
    }
}
void RawCapture::observeImpl()
{
    if (!active_)
        return;
    if (!IsWindow(parent_) || GetForegroundWindow() != parent_ || IsIconic(parent_))
    {
        stopImpl(true, "focus");
        return;
    }
    if (keyDown(VK_ESCAPE))
    {
        stopImpl(true, "escape");
        return;
    }
    POINT p{bounds_.left, bounds_.top};
    ClientToScreen(parent_, &p);
    RECT clip{};
    if (!GetClipCursor(&clip) || clip.left != p.x || clip.top != p.y || !EqualRect(&clip, &ownedClip_))
    {
        stopImpl(true, "bounds");
        return;
    }
    if (reportsMode_)
    {
        reportRecording_.progress.durationUs = nowUs() - startUs_;
        reportRecording_.progress.reports = reportRecording_.reports.size();
        if (onReportStatus)
            onReportStatus(reportRecording_.progress);
        return;
    }
    const auto us = nowUs() - startUs_;
    if (us > 15000000 || sampleCount_ + current_.samples.size() >= 1999999 || current_.samples.size() >= 200000)
    {
        stopImpl(false);
        return;
    }
    if (current_.samples.empty())
        return;
    const auto last = current_.samples.back();
    if (us - last.us >= 10000)
        current_.samples.push_back({us, last.x, last.y});
    if (us >= 6000000)
    {
        segment(true);
        if (onTrials && !finished_.empty())
        {
            auto ready = std::move(finished_);
            finished_.clear();
            onTrials(std::move(ready));
        }
        if (!active_)
            return;
        if (trialCount_ >= 1000)
        {
            stopImpl(false);
            return;
        }
    }
    if (onStatus)
        onStatus(active_, "采集中 · Esc 暂停", last.x, last.y);
}
void RawCapture::inputImpl(HRAWINPUT handle)
{
    if (!active_)
        return;
    if (GetForegroundWindow() != parent_)
    {
        stopImpl(true, "focus");
        return;
    }
    RAWINPUT input{};
    UINT size = sizeof(input);
    const auto read = GetRawInputData(handle, RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER));
    if (read == static_cast<UINT>(-1) || read < sizeof(RAWINPUTHEADER))
    {
        stopImpl(reportsMode_, "error");
        return;
    }
    if (input.header.dwType == RIM_TYPEMOUSE)
    {
        if (read < offsetof(RAWINPUT, data) + sizeof(RAWMOUSE))
        {
            stopImpl(reportsMode_, "error");
            return;
        }
        acceptInput(input);
    }
    // The current HRAWINPUT was already removed by GetMessage. Drain only the
    // remaining queued records, so it is neither lost nor counted twice.
    if (active_)
        inputBuffer_.drain([this](const RAWINPUT &next) { acceptInput(next); });
}
void RawCapture::acceptInput(const RAWINPUT &input)
{
    if (!active_)
        return;
    if (GetForegroundWindow() != parent_)
    {
        stopImpl(true, "focus");
        return;
    }
    if (reportsMode_)
    {
        auto &recording = reportRecording_;
        const auto source = reinterpret_cast<std::uintptr_t>(input.header.hDevice);
        if (input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
        {
            ++recording.progress.absoluteReports;
            return;
        }
        if (!source)
        {
            ++recording.progress.otherDeviceReports;
            return;
        }
        if (!device_)
        {
            if (!input.data.mouse.lLastX && !input.data.mouse.lLastY)
                return;
            device_ = source;
            recording.progress.device = std::to_string(source);
            UINT length = 0;
            if (GetRawInputDeviceInfoW(input.header.hDevice, RIDI_DEVICENAME, nullptr, &length) !=
                    static_cast<UINT>(-1) &&
                length && length < 32768)
            {
                std::wstring name(length, L'\0');
                if (GetRawInputDeviceInfoW(input.header.hDevice, RIDI_DEVICENAME, name.data(), &length) !=
                    static_cast<UINT>(-1))
                {
                    const auto end = name.find(L'\0');
                    if (end != std::wstring::npos)
                        name.resize(end);
                    recording.progress.device = application::storage::utf8(name);
                }
            }
        }
        if (source != device_)
        {
            ++recording.progress.otherDeviceReports;
            return;
        }
        recording.reports.push_back({nowUs() - startUs_, input.data.mouse.lLastX, input.data.mouse.lLastY,
                                     input.data.mouse.usFlags, input.data.mouse.usButtonFlags});
        if (recording.reports.size() >= application::MaximumMouseReports)
            stopImpl(true, "limit");
        return;
    }
    if (input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
    {
        stopImpl(false);
        return;
    }
    const auto source = reinterpret_cast<std::uintptr_t>(input.header.hDevice);
    if (!device_)
        device_ = source;
    if (device_ != source || current_.samples.empty() || current_.samples.size() >= 200000 ||
        sampleCount_ + current_.samples.size() >= 1999999)
    {
        stopImpl(false);
        return;
    }
    const auto last = current_.samples.back();
    const auto us = nowUs() - startUs_;
    current_.samples.push_back(
        {std::max(last.us, us), last.x + input.data.mouse.lLastX, last.y + input.data.mouse.lLastY});
}
} // namespace mousebox::desktop
