#pragma once
#include "application/ports/game_module.hpp"
#include <Windows.h>
#include <optional>
namespace mousebox::desktop
{
struct GameWindow
{
    HWND handle = nullptr;
    DWORD process = 0;
    RECT client{}; // Physical desktop coordinates; can be negative on another monitor.
};
bool matchesGameExecutable(std::wstring_view path, std::string_view expected) noexcept;
class ForegroundGameWindow
{
    HWND inspected_ = nullptr;
    DWORD process_ = 0;
    std::wstring image_;

  public:
    std::optional<GameWindow> sample(const application::ports::GameWindowTarget &);
};
} // namespace mousebox::desktop
