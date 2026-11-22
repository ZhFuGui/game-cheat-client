#pragma once
#include "domain/source_sample.hpp"
#include "render/scene_submission.hpp"

namespace mousebox::application::ports
{
// Associates Reader samples with a scene at the application boundary. The
// renderer receives only the scene and a completion callback.
struct PresentationFrame
{
    render::SceneSubmission scene;
    std::vector<domain::SourceSample> samples;
};
} // namespace mousebox::application::ports
