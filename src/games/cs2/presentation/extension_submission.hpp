#pragma once
#include "application/extensions/host.hpp"
#include "submission.hpp"

namespace mousebox::games::cs2
{
application::ports::PresentationFrame makeExtensionSubmission(const application::extensions::Input<Snapshot> &,
                                                const application::extensions::Host<Snapshot> &,
                                                const application::ports::PresentationOptions &, double metersPerUnit,
                                                std::uint64_t revision, std::uint64_t now,
                                                hitboxes::Display hitboxes = hitboxes::Display::Off);
} // namespace mousebox::games::cs2
