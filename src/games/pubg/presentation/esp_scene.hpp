#pragma once
#include "preview_scene.hpp"
#include "render/esp_bearing.hpp"
#include "domain/scene/observations.hpp"
namespace mousebox::render
{
class EspProjector
{
    std::shared_ptr<const control::Frame> previous_, previousView_;
    std::shared_ptr<const scene::Snapshot> geometry_, cameraScene_;
    void connect(const std::shared_ptr<const control::Frame> &);

  public:
    std::shared_ptr<const EspPrimitives> project(const Scene &, float width, float height);
};
} // namespace mousebox::render
