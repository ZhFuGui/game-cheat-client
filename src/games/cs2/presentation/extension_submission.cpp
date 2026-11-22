#include "extension_submission.hpp"
#include "games/cs2/extensions/cover/result.hpp"
#include "games/cs2/extensions/cover/definition.hpp"
#include "games/cs2/hitboxes/frame.hpp"
#include "games/cs2/extensions/remaining_damage/definition.hpp"
#include "games/cs2/extensions/remaining_damage/result.hpp"
#include "games/cs2/extensions/head_damage/definition.hpp"
#include "games/cs2/extensions/head_damage/result.hpp"
#include "games/cs2/extensions/best_damage/definition.hpp"
#include "games/cs2/extensions/best_damage/result.hpp"

namespace mousebox::games::cs2
{
application::ports::PresentationFrame makeExtensionSubmission(const application::extensions::Input<Snapshot> &input,
                                                const application::extensions::Host<Snapshot> &host,
                                                const application::ports::PresentationOptions &options, double scale,
                                                std::uint64_t revision, std::uint64_t now, hitboxes::Display display)
{
    std::shared_ptr<const extensions::BestDamageResult> bestDamage;
    if (input.frame)
        if (const auto ready = host.latestFor(extensions::bestDamageDefinition().id, input.scope, now))
        {
            bestDamage = ready->find<extensions::BestDamageResult>(extensions::bestDamageDefinition().id);
            if (bestDamage && !extensions::matchesCurrent(*bestDamage, *input.frame))
                bestDamage.reset();
        }
    if (input.frame)
        if (auto enriched = host.latest(input.scope, now, 250);
            enriched && enriched->input.capture <= input.capture &&
            (display == hitboxes::Display::Off ||
             (enriched->input.frame && hitboxes::retainsEligibility(*enriched->input.frame, *input.frame))))
        {
            auto cover = enriched->find<extensions::CoverResult>(extensions::coverDefinition().id);
            auto damage = enriched->find<extensions::RemainingDamageResult>(extensions::remainingDamageDefinition().id);
            auto headDamage = enriched->find<extensions::HeadDamageResult>(extensions::headDamageDefinition().id);
            if (damage && !extensions::matchesCurrent(*damage, *input.frame))
                damage.reset();
            if (headDamage && !extensions::matchesCurrent(*headDamage, *input.frame))
                headDamage.reset();
            if (cover || damage || headDamage)
                // Render the analyzed snapshot, not old annotations on a newer
                // actor pose/matrix. The original source sample follows it to GPU completion.
                return makeSubmission(enriched->input.frame, options, scale, revision, enriched->input.sample,
                                      std::move(cover), display, std::move(damage), std::move(headDamage),
                                      std::move(bestDamage), enriched->input.receipt);
        }
    // Independent best-point completion never selects its old analyzed frame.
    // Keep drawing the current source while the next result is still running.
    return makeSubmission(input.frame, options, scale, revision, input.sample, {}, display, {}, {},
                          std::move(bestDamage), input.receipt);
}
} // namespace mousebox::games::cs2
