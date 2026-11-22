#include "capture_client.hpp"
#include "capture_payload.hpp"
#include <chrono>
#include <stdexcept>

namespace mousebox::desktop
{
using capture::Json;
CaptureClient::CaptureClient(DWORD parentPid, std::string token)
    : parentPid_(parentPid), token_(std::move(token)), cancel_(CreateEventW(nullptr, TRUE, FALSE, nullptr))
{
    if (!cancel_)
        throw std::runtime_error("原生采集客户端创建失败");
}
CaptureClient::~CaptureClient()
{
    closing_ = true;
    SetEvent(cancel_.get());
    if (reader_.joinable())
        reader_.join();
}
void CaptureClient::connect()
{
    if (token_.empty())
        return; // CLI and isolated bridge tests never register a mouse receiver.
    commands_ = capture::connect(capture::pipeName(parentPid_, token_, L"-commands"), cancel_.get(), parentPid_);
    events_ = capture::connect(capture::pipeName(parentPid_, token_, L"-events"), cancel_.get(), parentPid_);
    connected_ = true;
    reader_ = std::jthread([this] { receive(); });
}
Json CaptureClient::call(Json command)
{
    const std::lock_guard serialize(commandMutex_);
    if (!connected_ || failed_ || closing_)
        throw std::runtime_error("前台原生采集模块不可用，请重新启动软件");
    Json reply;
    try
    {
        capture::write(commands_.get(), cancel_.get(), {std::move(command), {}});
        reply = capture::read(commands_.get(), cancel_.get(), 4000).metadata;
    }
    catch (...)
    {
        failed_ = true;
        SetEvent(cancel_.get());
        // Closing the command stream also tells the in-process server to stop
        // capture and release its owned cursor clip, even if JS is unresponsive.
        commands_ = capture::Handle{};
        delivered_.notify_all();
        throw;
    }
    const auto through = reply.at("through").get<std::uint64_t>();
    {
        std::unique_lock lock(eventMutex_);
        if (!delivered_.wait_for(lock, std::chrono::seconds(4),
                                 [this, through] { return sequence_ >= through || failed_; }) ||
            failed_)
        {
            failed_ = true;
            SetEvent(cancel_.get());
            commands_ = capture::Handle{};
            throw std::runtime_error("原生采集完成数据尚未交付");
        }
    }
    if (!reply.at("ok").get<bool>())
        throw std::runtime_error(reply.at("error").get<std::string>());
    return reply.at("result");
}
void CaptureClient::setBounds(HWND parent, RECT rect)
{
    if (token_.empty())
        return;
    call({{"command", "bounds"},
          {"parent", reinterpret_cast<std::uintptr_t>(parent)},
          {"bounds", {rect.left, rect.top, rect.right, rect.bottom}}});
}
void CaptureClient::start(unsigned dpi)
{
    call({{"command", "learning.start"}, {"dpi", dpi}});
}
void CaptureClient::startReports()
{
    call({{"command", "reports.start"}});
}
void CaptureClient::stop(bool keep, std::string reason)
{
    if (!connected_ || failed_ || closing_)
        return;
    call({{"command", "stop"}, {"keep", keep}, {"reason", std::move(reason)}});
}
void CaptureClient::clear()
{
    if (!token_.empty())
        call({{"command", "clear"}});
}
Json CaptureClient::inspect()
{
    if (token_.empty())
        return {{"connected", false}, {"method", "unavailable"}};
    auto value = call({{"command", "inspect"}});
    value["connected"] = true;
    return value;
}
void CaptureClient::accept(const capture::Frame &frame)
{
    const auto &value = frame.metadata;
    const auto event = value.at("event").get<std::string>();
    if (event == "learning.status" && onStatus)
        onStatus(value.at("active").get<bool>(), value.at("text").get<std::string>(), value.at("x").get<double>(),
                 value.at("y").get<double>());
    else if (event == "learning.trials" && onTrials)
        onTrials(capture::decodeTrials(frame));
    else if (event == "reports.progress" && onReportStatus)
        onReportStatus(capture::decodeProgress(value));
    else if (event == "reports.complete" && onReports)
        onReports(capture::decodeReports(frame));
    else
        throw std::runtime_error("未知原生采集事件");
}
void CaptureClient::receive()
{
    try
    {
        while (!closing_)
        {
            const auto frame = capture::read(events_.get(), cancel_.get());
            const auto sequence = frame.metadata.at("sequence").get<std::uint64_t>();
            {
                const std::lock_guard lock(eventMutex_);
                if (sequence != sequence_ + 1)
                    throw std::runtime_error("原生采集事件次序不符");
            }
            accept(frame);
            {
                const std::lock_guard lock(eventMutex_);
                sequence_ = sequence;
            }
            delivered_.notify_all();
        }
    }
    catch (const std::exception &e)
    {
        if (!closing_)
        {
            failed_ = true;
            SetEvent(cancel_.get());
            delivered_.notify_all();
            // The reader may fail while another thread awaits a completion
            // barrier. Wake it before taking the command lock, then close the
            // command stream so the server releases any active capture.
            {
                const std::lock_guard serialize(commandMutex_);
                commands_ = capture::Handle{};
            }
            try
            {
                if (onStatus)
                    onStatus(false, e.what(), 0, 0);
                if (onReportStatus)
                {
                    application::MouseReportsProgress progress;
                    progress.reason = "error";
                    onReportStatus(std::move(progress));
                }
            }
            catch (...)
            {
            }
        }
    }
}
} // namespace mousebox::desktop
