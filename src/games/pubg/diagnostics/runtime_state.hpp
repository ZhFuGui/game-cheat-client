#pragma once
#include "application/json.hpp"
#include "games/pubg/runtime/runtime_models.hpp"
#include "render/directx_renderer.hpp"

namespace mousebox::application
{
struct RuntimePresentation
{
    bool sessionEligible;
    const render::RenderStats &renderer;
    const std::string &ballisticsError;
};
// Projection only: no devices, filesystem writes, locks or clock reads.
Json describeRuntimeState(const Snapshot &, const control::LiveDiagnostic &, std::uint64_t now,
                          const RuntimePresentation &);
} // namespace mousebox::application
