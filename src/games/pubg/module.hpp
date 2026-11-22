#pragma once
#include "games/pubg/model/control_frame.hpp"
#include "domain/scene/observations.hpp"

namespace mousebox::games::pubg
{
// Sole interpretation point for the existing PUBG camera, map scale and native
// skeleton layout. A supported module does not grant current output eligibility.
std::shared_ptr<const scene::Snapshot> observeScene(const control::Frame &);
} // namespace mousebox::games::pubg
