#pragma once
#include "application/json.hpp"
#include "application/output/device_snapshot.hpp"

namespace mousebox::render
{
struct RenderStats;
}
namespace mousebox::application
{
struct DevicePresentation
{
    const Json &ports;
    bool portsChecked;
    const std::string &portsError;
    const render::RenderStats &renderer;
    const std::string &graphicsError;
};
Json describeDeviceConnections(const DeviceSnapshot &, const Json &ports, bool checked, const std::string &error,
                               std::uint64_t now);
Json describeDevices(const OutputObservation &, std::uint64_t now, const DevicePresentation &);
} // namespace mousebox::application
