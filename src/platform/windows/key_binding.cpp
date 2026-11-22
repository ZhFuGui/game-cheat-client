#include "key_binding.hpp"
#include <array>
namespace mousebox
{
std::optional<unsigned> localKeyCode(std::string_view id) noexcept
{
    if (id.size() == 4 && id.starts_with("Key") && id[3] >= 'A' && id[3] <= 'Z')
        return static_cast<unsigned>(id[3]);
    if (id.size() == 6 && id.starts_with("Digit") && id[5] >= '0' && id[5] <= '9')
        return static_cast<unsigned>(id[5]);
    constexpr std::array<std::pair<std::string_view, unsigned>, 23> keys{
        {{"F1", 0x70},         {"F2", 0x71},          {"F3", 0x72},           {"F4", 0x73},      {"F5", 0x74},
         {"F6", 0x75},         {"F7", 0x76},          {"F9", 0x78},           {"F10", 0x79},     {"F11", 0x7a},
         {"F12", 0x7b},        {"Space", 0x20},       {"Tab", 0x09},          {"Enter", 0x0d},   {"ShiftLeft", 0xa0},
         {"ShiftRight", 0xa1}, {"ControlLeft", 0xa2}, {"ControlRight", 0xa3}, {"AltLeft", 0xa4}, {"AltRight", 0xa5},
         {"ArrowUp", 0x26},    {"ArrowDown", 0x28},   {"ArrowLeft", 0x25}}};
    if (id == "ArrowRight")
        return 0x27;
    for (const auto &[key, code] : keys)
        if (key == id)
            return code;
    return {};
}
} // namespace mousebox
