#include "serial_session.hpp"
#include "serial_sender.hpp"
#include "platform/windows/ports.hpp"
#include <exception>
#include <utility>

namespace mousebox
{
SerialSession::~SerialSession()
{
    disconnect();
}
void SerialSession::connect(std::wstring requested, Opened opened, Run run, Fault fault)
{
    disconnect();
    worker_ = std::jthread([requested = std::move(requested), opened = std::move(opened), run = std::move(run),
                            fault = std::move(fault)](std::stop_token stop) {
        try
        {
            SerialSender link(chooseTtlPort(requested), stop);
            opened(link.baudRate(), link.motionLimits());
            run(link, stop);
            link.stop(0);
            link.drain(std::chrono::milliseconds(200));
        }
        catch (const std::exception &error)
        {
            fault(error.what());
        }
    });
}
void SerialSession::disconnect()
{
    worker_.request_stop();
    if (worker_.joinable())
        worker_.join();
}
} // namespace mousebox
