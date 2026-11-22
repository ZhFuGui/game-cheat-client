#pragma once
#include "definition.hpp"
#include "result.hpp"
#include "games/cs2/extensions/ballistics_session.hpp"
namespace mousebox::games::cs2::extensions
{
class RemainingDamageExtension final : public application::extensions::Extension<Snapshot>
{
    BallisticsSession scene_;

  public:
    explicit RemainingDamageExtension(std::shared_ptr<application::ports::BallisticsProvider>);
    const application::extensions::Definition &definition() const noexcept override;
    application::extensions::Contribution evaluate(const application::extensions::Input<Snapshot> &,
                                                   std::stop_token) override;
    void reset() noexcept override;
};
} // namespace mousebox::games::cs2::extensions
