#pragma once
#include <cstdint>
#include <string>

namespace mousebox::control
{
enum class IdentityKind
{
    NativeLifecycle, // Historical offline data, never synthesized from Consumer tokens.
    EntityToken,
    ControllerToken,
    WeaponToken
};
struct Identity
{
    std::uint64_t id = 0, actor = 0;
    std::int32_t index = 0;
    std::uint32_t serial = 0;
    IdentityKind kind = IdentityKind::NativeLifecycle;
    std::uint64_t token = 0;
    bool operator==(const Identity &) const = default;
    static Identity entity(std::uint64_t value) noexcept
    {
        return {value, 0, 0, 0, IdentityKind::EntityToken, value};
    }
    static Identity controller(std::uint64_t value) noexcept
    {
        return {0, 0, 0, 0, IdentityKind::ControllerToken, value};
    }
    static Identity weapon(std::uint64_t value) noexcept
    {
        return {0, 0, 0, 0, IdentityKind::WeaponToken, value};
    }
};
inline bool hasLifecycle(const Identity &value) noexcept
{
    if (value.kind == IdentityKind::NativeLifecycle)
        return value.actor != 0 && value.index >= 0;
    if (value.kind != IdentityKind::EntityToken && value.kind != IdentityKind::ControllerToken &&
        value.kind != IdentityKind::WeaponToken)
        return false;
    return value.token != 0 && value.actor == 0 && value.index == 0 && value.serial == 0 &&
           (value.kind == IdentityKind::EntityToken ? value.id == value.token : value.id == 0);
}
inline bool sameLifecycle(const Identity &a, const Identity &b) noexcept
{
    if (a.kind != b.kind)
        return false;
    if (a.kind != IdentityKind::NativeLifecycle)
        return hasLifecycle(a) && hasLifecycle(b) && a.token == b.token;
    return a.actor == b.actor && a.index == b.index && a.serial == b.serial;
}
inline bool sameNativeAddress(const Identity &a, const Identity &b) noexcept
{
    return a.kind == IdentityKind::NativeLifecycle && b.kind == IdentityKind::NativeLifecycle && a.actor != 0 &&
           a.actor == b.actor;
}
inline std::string lifecycleKey(const Identity &value)
{
    switch (value.kind)
    {
    case IdentityKind::EntityToken:
        return "entity:" + std::to_string(value.token);
    case IdentityKind::ControllerToken:
        return "controller:" + std::to_string(value.token);
    case IdentityKind::WeaponToken:
        return "weapon:" + std::to_string(value.token);
    case IdentityKind::NativeLifecycle:
        return std::to_string(value.actor) + ":" + std::to_string(value.index) + ":" + std::to_string(value.serial);
    }
    return {};
}
} // namespace mousebox::control
