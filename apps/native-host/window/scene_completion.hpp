#pragma once
#include "application/ports/presentation_frame.hpp"
#include <span>
#include <utility>

namespace mousebox::desktop
{
using DrawCompletion = std::function<void(std::span<const domain::SourceSample>, double)>;

inline render::SceneSubmission withDrawCompletion(application::ports::PresentationFrame frame,
                                                  const DrawCompletion &completed)
{
    if (completed)
        frame.scene.completed = [notify = completed, samples = std::move(frame.samples)](double now) {
            notify(samples, now);
        };
    return std::move(frame.scene);
}
} // namespace mousebox::desktop
