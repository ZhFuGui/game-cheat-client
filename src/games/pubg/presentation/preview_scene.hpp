#pragma once
#include "games/pubg/model/control_frame.hpp"
#include "games/pubg/model/esp_layers.hpp"
#include "render/primitives.hpp"
#include <array>
#include <memory>
#include <utility>

namespace mousebox::render
{
struct Scene
{
    std::shared_ptr<const control::Frame> living, view;
    std::optional<control::Target> target;
    bool showSelf = false;
    std::uint64_t revision = 0;
    Palette palette;
    bool english = false;
    std::optional<EspOptions> esp;
    std::shared_ptr<const control::EspTeamInfo> espTeam;
    std::optional<control::Identity> heldTarget;
    double aimConeDegrees = 0;
    std::optional<unsigned> spectators;
};
class EspProjector;
// Cache connectivity on Living changes; View-only updates reuse it. Projection
// computes the camera basis once, rather than atan/acos/tan for every bone.
class SceneProjector
{
    std::shared_ptr<const control::Frame> previous_;
    std::vector<std::vector<std::pair<std::size_t, std::size_t>>> links_;
    std::shared_ptr<EspProjector> esp_;

  public:
    Primitives project(const Scene &, float width, float height);
};
} // namespace mousebox::render
