#pragma once
#include "application/ports/output_link.hpp"
#include <functional>
#include <string>
#include <thread>

namespace mousebox
{
// Owns the open serial link and the only worker that can produce its output.
class SerialSession
{
    std::jthread worker_;

  public:
    using Opened = std::function<void(unsigned baud, control::DeviceMotionLimits)>;
    using Run = std::function<void(application::ports::OutputLink &, std::stop_token)>;
    using Fault = std::function<void(const std::string &)>;
    ~SerialSession();
    void connect(std::wstring requested, Opened, Run, Fault);
    void disconnect();
};
} // namespace mousebox
