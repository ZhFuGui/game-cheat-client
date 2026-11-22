#pragma once
#include "capture_channel.hpp"
#include "raw_capture.hpp"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

namespace mousebox::desktop
{
// Loaded inside Electron's window-owning process. All callbacks, capture and
// completed raw payloads stay in C++; Node only opens/inspects/closes this endpoint.
class CaptureServer
{
    capture::Handle cancel_, commands_, events_;
    std::filesystem::path executable_;
    std::unique_ptr<RawCapture> capture_;
    std::mutex mutex_;
    std::condition_variable wake_;
    std::deque<capture::Frame> outgoing_;
    std::size_t queuedBytes_ = 0;
    std::uint64_t sequence_ = 0;
    std::atomic<std::uint64_t> lastProgressMs_ = 0;
    std::atomic<bool> closing_ = false, failed_ = false;
    std::jthread commandThread_, eventThread_;
    void send(capture::Frame);
    void fail() noexcept;
    void commands();
    void events();
    capture::Json execute(const capture::Json &);

  public:
    CaptureServer(const std::string &token, std::filesystem::path executable);
    ~CaptureServer();
    void close() noexcept;
    capture::Json inspect() const;
};
} // namespace mousebox::desktop
