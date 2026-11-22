#include "game_window.hpp"
#include <algorithm>
#include <cwctype>
namespace mousebox::desktop
{
bool matchesGameExecutable(std::wstring_view path, std::string_view expected) noexcept
{
    const auto separator = path.find_last_of(L"\\/");
    const auto name = separator == path.npos ? path : path.substr(separator + 1);
    if (expected.empty() || name.size() != expected.size())
        return false;
    for (std::size_t i = 0; i < name.size(); ++i)
        if (std::towlower(name[i]) != std::towlower(static_cast<unsigned char>(expected[i])))
            return false;
    return true;
}
std::optional<GameWindow> ForegroundGameWindow::sample(const application::ports::GameWindowTarget &expected)
{
    const auto window = GetForegroundWindow();
    DWORD process = 0;
    if (!window || !GetWindowThreadProcessId(window, &process) || process == GetCurrentProcessId() ||
        (expected.process && process != expected.process) || !IsWindowVisible(window) || IsIconic(window))
        return {};
    if (window != inspected_ || process != process_)
    {
        inspected_ = window;
        process_ = process;
        image_.clear();
        const auto handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process);
        if (handle)
        {
            std::wstring path(32768, L'\0');
            DWORD count = static_cast<DWORD>(path.size());
            if (QueryFullProcessImageNameW(handle, 0, path.data(), &count))
            {
                path.resize(count);
                image_ = std::move(path);
            }
            CloseHandle(handle);
        }
    }
    if (!matchesGameExecutable(image_, expected.executable))
        return {};
    RECT rect{};
    if (!GetClientRect(window, &rect))
        return {};
    POINT a{rect.left, rect.top}, b{rect.right, rect.bottom};
    if (!ClientToScreen(window, &a) || !ClientToScreen(window, &b) || b.x <= a.x || b.y <= a.y || b.x - a.x > 32768 ||
        b.y - a.y > 32768)
        return {};
    return GameWindow{window, process, {a.x, a.y, b.x, b.y}};
}
} // namespace mousebox::desktop
