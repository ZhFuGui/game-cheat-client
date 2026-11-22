#pragma once
#include "application/extensions/contracts.hpp"
#include "application/ports/shot_prediction.hpp"
#include "games/cs2/model/snapshot.hpp"

namespace mousebox::games::cs2::extensions
{
// Each query keeps its own retry state; the provider shares the actual scene.
class BallisticsSession
{
    std::shared_ptr<application::ports::BallisticsProvider> provider_;
    std::shared_ptr<const application::ports::BallisticsScene> scene_;
    std::optional<application::extensions::Scope> scope_;
    unsigned build_ = 0;
    std::uint64_t retryAtMs_ = 0;
    std::string error_;

  public:
    explicit BallisticsSession(std::shared_ptr<application::ports::BallisticsProvider>);
    bool available() const noexcept;
    void retainScope(const application::extensions::Scope &, unsigned build) noexcept;
    std::shared_ptr<const application::ports::BallisticsScene> acquire(const application::extensions::Scope &,
                                                                       const MapIdentity &, unsigned build,
                                                                       std::uint64_t observedAtMs);
    const std::string &error() const noexcept;
    void reset() noexcept;
};
} // namespace mousebox::games::cs2::extensions
