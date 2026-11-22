#include "capture_server.hpp"
#include "capture_payload.hpp"
#include <chrono>
#include <stdexcept>

namespace mousebox::desktop
{
using capture::Frame;
using capture::Json;
CaptureServer::CaptureServer(const std::string &token, std::filesystem::path executable)
    : cancel_(CreateEventW(nullptr, TRUE, FALSE, nullptr)),
      commands_(capture::createServer(capture::pipeName(GetCurrentProcessId(), token, L"-commands"))),
      events_(capture::createServer(capture::pipeName(GetCurrentProcessId(), token, L"-events"))),
      executable_(std::move(executable)), capture_(std::make_unique<RawCapture>(nullptr, GetCurrentProcessId()))
{
    if (!cancel_)
        throw std::runtime_error("原生采集取消事件创建失败");
    capture_->onStatus = [this](bool active, std::string text, double x, double y) {
        send({{{"event", "learning.status"}, {"active", active}, {"text", std::move(text)}, {"x", x}, {"y", y}}, {}});
    };
    capture_->onTrials = [this](auto trials) { send(capture::encodeTrials(trials)); };
    capture_->onReportStatus = [this](auto progress) {
        const auto now = GetTickCount64();
        if (!progress.recording || !lastProgressMs_ || now - lastProgressMs_ >= 100)
        {
            lastProgressMs_ = now;
            send(capture::encodeProgress(progress));
        }
    };
    capture_->onReports = [this](auto recording) { send(capture::encodeReports(recording)); };
    commandThread_ = std::jthread([this] { commands(); });
    eventThread_ = std::jthread([this] { events(); });
}
CaptureServer::~CaptureServer()
{
    close();
}
void CaptureServer::close() noexcept
{
    if (closing_.exchange(true))
        return;
    SetEvent(cancel_.get());
    wake_.notify_all();
    if (commandThread_.joinable())
        commandThread_.join();
    if (eventThread_.joinable())
        eventThread_.join();
    commands_ = capture::Handle{};
    events_ = capture::Handle{};
    try
    {
        capture_->stop();
    }
    catch (...)
    {
    }
    capture_.reset();
}
void CaptureServer::fail() noexcept
{
    if (failed_.exchange(true))
        return;
    SetEvent(cancel_.get());
    wake_.notify_all();
    DisconnectNamedPipe(commands_.get());
    DisconnectNamedPipe(events_.get());
    try
    {
        capture_->stop();
    }
    catch (...)
    {
    }
}
Json CaptureServer::inspect() const
{
    DWORD foreground = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &foreground);
    return {{"processId", GetCurrentProcessId()},        {"foregroundProcessId", foreground},
            {"active", capture_ && capture_->active()},  {"failed", failed_.load()},
            {"method", "foreground-buffered-raw-input"}, {"rawReportsThroughJs", false}};
}
void CaptureServer::send(Frame frame)
{
    if (closing_ || failed_)
        return;
    std::unique_lock lock(mutex_);
    if (outgoing_.size() >= 32 || queuedBytes_ + frame.bytes.size() > capture::MaximumPayload)
    {
        lock.unlock();
        fail();
        throw std::runtime_error("原生采集完成队列已满");
    }
    frame.metadata["sequence"] = ++sequence_;
    queuedBytes_ += frame.bytes.size();
    outgoing_.push_back(std::move(frame));
    wake_.notify_one();
}
Json CaptureServer::execute(const Json &request)
{
    const auto command = request.at("command").get<std::string>();
    if (command == "inspect")
        return inspect();
    if (command == "bounds")
    {
        const auto handle = request.at("parent").get<std::uint64_t>();
        const auto &rect = request.at("bounds");
        if (!rect.is_array() || rect.size() != 4)
            throw std::runtime_error("原生采集区域不合法");
        capture_->setBounds(reinterpret_cast<HWND>(static_cast<std::uintptr_t>(handle)),
                            {rect[0].get<LONG>(), rect[1].get<LONG>(), rect[2].get<LONG>(), rect[3].get<LONG>()});
    }
    else if (command == "learning.start")
        capture_->start(request.at("dpi").get<unsigned>());
    else if (command == "reports.start")
    {
        lastProgressMs_ = 0;
        capture_->startReports();
    }
    else if (command == "stop")
        capture_->stop(request.value("keep", true), request.value("reason", std::string("manual")));
    else if (command == "clear")
        capture_->clear();
    else
        throw std::runtime_error("未知原生采集命令");
    return {{"ok", true}};
}
void CaptureServer::commands()
{
    try
    {
        capture::acceptClient(commands_.get(), cancel_.get(), executable_);
        while (!closing_ && !failed_)
        {
            const auto frame = capture::read(commands_.get(), cancel_.get());
            Json reply;
            try
            {
                if (!frame.bytes.empty())
                    throw std::runtime_error("采集命令不能携带原始报文");
                reply = {{"ok", true}, {"result", execute(frame.metadata)}};
            }
            catch (const std::exception &e)
            {
                reply = {{"ok", false}, {"error", e.what()}};
            }
            {
                const std::lock_guard lock(mutex_);
                reply["through"] = sequence_;
            }
            capture::write(commands_.get(), cancel_.get(), {std::move(reply), {}});
        }
    }
    catch (...)
    {
        if (!closing_)
            fail();
    }
}
void CaptureServer::events()
{
    try
    {
        capture::acceptClient(events_.get(), cancel_.get(), executable_);
        while (!closing_ && !failed_)
        {
            Frame frame;
            {
                std::unique_lock lock(mutex_);
                wake_.wait(lock, [this] { return !outgoing_.empty() || closing_ || failed_; });
                if (closing_ || failed_)
                    return;
                frame = std::move(outgoing_.front());
                outgoing_.pop_front();
                queuedBytes_ -= frame.bytes.size();
            }
            capture::write(events_.get(), cancel_.get(), frame);
        }
    }
    catch (...)
    {
        if (!closing_)
            fail();
    }
}
} // namespace mousebox::desktop
