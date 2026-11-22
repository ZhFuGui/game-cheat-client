#pragma once
#include "application/json.hpp"
#include <string_view>
#include <string>

namespace mousebox::application::ports
{
// Configuration has its own schema; game modules cannot inherit another game's
// defaults just because both expose an ESP or a mouse output device.
class GameConfiguration
{
  public:
    virtual ~GameConfiguration() = default;
    virtual std::string_view gameId() const noexcept = 0;
    virtual Json defaults() const = 0;
    virtual Json normalize(const Json &) const = 0;
    // Only explicitly additive settings may be absent in an older document.
    // Present values still pass strict validation; other missing keys are errors.
    virtual bool permitsAbsentValue(std::string_view) const noexcept
    {
        return false;
    }
    virtual void validateValue(std::string_view key, const Json &) const = 0;
    // A single user edit may also disable conflicting features. The game owns
    // that policy; the host applies and persists the resulting values together.
    virtual Json updatedValues(const Json &current, std::string_view key, const Json &value) const
    {
        validateValue(key, value);
        auto next = current;
        next[std::string(key)] = value;
        return next;
    }
    // Persisted documents may contain explicitly retired keys. They are
    // validated before normalize removes them; live settings stay strict.
    virtual void validateStoredValue(std::string_view key, const Json &value) const
    {
        validateValue(key, value);
    }
    virtual void validateEspValue(std::string_view key, const Json &) const = 0;
};
} // namespace mousebox::application::ports
