#pragma once
#include "games/cs2/model/snapshot.hpp"
#include "application/ports/game_module.hpp"
#include "games/cs2/hitboxes/assets.hpp"
namespace mousebox::games::cs2
{
namespace extensions
{
struct CoverResult;
struct RemainingDamageResult;
struct HeadDamageResult;
struct BestDamageResult;
} // namespace extensions
std::optional<render::Point> projectPoint(const std::array<double, 16> &, Vec3, float width, float height);
application::ports::PresentationFrame makeSubmission(std::shared_ptr<const Snapshot>, const application::ports::PresentationOptions &,
                                       double metersPerUnit, std::uint64_t revision,
                                       std::optional<domain::SourceSample> sample = {},
                                       std::shared_ptr<const extensions::CoverResult> cover = {},
                                       hitboxes::Display hitboxes = hitboxes::Display::Off,
                                       std::shared_ptr<const extensions::RemainingDamageResult> damage = {},
                                       std::shared_ptr<const extensions::HeadDamageResult> headDamage = {},
                                       std::shared_ptr<const extensions::BestDamageResult> bestDamage = {},
                                       std::optional<domain::SourceReceipt> receipt = {});
} // namespace mousebox::games::cs2
