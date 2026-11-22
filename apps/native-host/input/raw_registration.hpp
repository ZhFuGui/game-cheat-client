#pragma once
#include <Windows.h>
#include <optional>

namespace mousebox::desktop
{
class RawRegistration
{
    HWND receiver_ = nullptr;
    std::optional<RAWINPUTDEVICE> previous_;

  public:
    void acquire(HWND);
    void release() noexcept;
};
} // namespace mousebox::desktop
