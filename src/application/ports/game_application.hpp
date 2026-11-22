#pragma once
#include "application/json.hpp"
#include "control/human_motion.hpp"
#include "domain/source_sample.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace mousebox::application::ports
{
// Optional explicit application adapter for a game. Legacy PUBG is migrated
// separately; new games never enter its settings/frame/diagnostic functions.
class GameApplication
{
  public:
    virtual ~GameApplication() = default;
    virtual void configure(const Json &values) = 0;
    // Returns this game's state, algorithm and algorithmSections only. Global
    // device facts and the workspace envelope remain owned by the host.
    virtual Json snapshot(bool diagnostics) const = 0;
    // Explicit game.* commands only; cannot override stop, device or lifecycle.
    virtual std::optional<Json> command(std::string_view, const Json &) = 0;
    virtual void setHumanModel(std::shared_ptr<const control::HumanMotionModel>, unsigned dpi, bool variation) = 0;
    virtual void setSceneNotifier(std::function<void()>) = 0;
    virtual void drawCompleted(std::span<const domain::SourceSample>, double completedAtMs) = 0;
};
} // namespace mousebox::application::ports
