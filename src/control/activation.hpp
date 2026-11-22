#pragma once
#include "button_feedback.hpp"
#include <optional>

namespace mousebox::control
{
enum class ActivationSource : unsigned
{
    DeviceMouse,
    LocalKeyboard,
    Continuous
};
struct ActivationBinding
{
    ActivationSource source = ActivationSource::DeviceMouse;
    unsigned code = 8;
    bool operator==(const ActivationBinding &) const = default;
    bool sideButton() const noexcept
    {
        return source == ActivationSource::DeviceMouse && (code == 8 || code == 16);
    }
};
inline bool activationDown(ActivationBinding b, const ButtonObservation &buttons, bool localKeyDown) noexcept
{
    return b.source == ActivationSource::Continuous      ? true
           : b.source == ActivationSource::LocalKeyboard ? localKeyDown
                                                         : buttons.down(static_cast<std::uint16_t>(b.code));
}
inline std::optional<unsigned> activationRelease(ActivationBinding b, const ButtonObservation &buttons) noexcept
{
    return b.sideButton() ? std::optional(buttons.releases(static_cast<std::uint16_t>(b.code))) : std::nullopt;
}
} // namespace mousebox::control
