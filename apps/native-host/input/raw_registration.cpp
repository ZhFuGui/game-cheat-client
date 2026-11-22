#include "raw_registration.hpp"
#include <stdexcept>
#include <vector>

namespace mousebox::desktop
{
namespace
{
std::optional<RAWINPUTDEVICE> mouseRegistration()
{
    UINT count = 0;
    if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == static_cast<UINT>(-1))
        throw std::runtime_error("无法查询已有原始鼠标注册");
    std::vector<RAWINPUTDEVICE> values(count);
    if (count && GetRegisteredRawInputDevices(values.data(), &count, sizeof(RAWINPUTDEVICE)) == static_cast<UINT>(-1))
        throw std::runtime_error("无法读取已有原始鼠标注册");
    for (const auto &value : values)
        if (value.usUsagePage == 1 && value.usUsage == 2)
            return value;
    return {};
}
} // namespace
void RawRegistration::acquire(HWND receiver)
{
    release();
    previous_ = mouseRegistration();
    // Capture only while our Electron process owns the foreground window. Keep
    // legacy delivery so the browser's pointer and finish button remain usable.
    RAWINPUTDEVICE device{1, 2, 0, receiver};
    if (!RegisterRawInputDevices(&device, 1, sizeof(device)))
        throw std::runtime_error("前台原始鼠标输入注册失败");
    receiver_ = receiver;
}
void RawRegistration::release() noexcept
{
    if (!receiver_)
        return;
    try
    {
        const auto current = mouseRegistration();
        // Chromium or another owner may have since registered its own receiver.
        // Never remove that newer registration.
        if (current && current->hwndTarget == receiver_)
        {
            auto restore = previous_.value_or(RAWINPUTDEVICE{1, 2, RIDEV_REMOVE, nullptr});
            RegisterRawInputDevices(&restore, 1, sizeof(restore));
        }
    }
    catch (...)
    {
    }
    receiver_ = nullptr;
    previous_.reset();
}
} // namespace mousebox::desktop
