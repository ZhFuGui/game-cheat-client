#pragma once
#include "definition.hpp"
#include "result.hpp"
#include "games/cs2/extensions/ballistics_session.hpp"
#include <map>
namespace mousebox::games::cs2::extensions
{
class BestDamageExtension final : public application::extensions::Extension<Snapshot>
{
    BallisticsSession scene_;
    std::optional<application::extensions::Scope> scope_;
    std::optional<ShotContext> shooter_;
    std::map<std::pair<std::uint32_t, std::uint32_t>, damage_search::Seed> seeds_;
    std::size_t nextActor_ = 0;

  public:
    explicit BestDamageExtension(std::shared_ptr<application::ports::BallisticsProvider>);
    const application::extensions::Definition &definition() const noexcept override;
    bool independentWorker() const noexcept override
    {
        return true;
    }
    unsigned maximumResultAgeMs() const noexcept override
    {
        return 1000;
    }
    application::extensions::Contribution evaluate(const application::extensions::Input<Snapshot> &,
                                                   std::stop_token) override;
    void reset() noexcept override;
};
} // namespace mousebox::games::cs2::extensions
