#pragma once
#include "definition.hpp"
#include "application/ports/scene_visibility.hpp"
#include "games/cs2/model/snapshot.hpp"

namespace mousebox::games::cs2::extensions
{
class CoverExtension final : public application::extensions::Extension<Snapshot>
{
    std::shared_ptr<application::ports::VisibilityProvider> provider_;
    std::shared_ptr<const application::ports::VisibilityScene> scene_;
    std::optional<application::extensions::Scope> loadedScope_;
    std::uint64_t retryAtMs_ = 0;
    std::string loadError_;
    bool hitboxes_ = false;

  public:
    explicit CoverExtension(std::shared_ptr<application::ports::VisibilityProvider> provider);
    const application::extensions::Definition &definition() const noexcept override;
    application::Json settings(const application::Json &) const override;
    void configure(const application::Json &) override;
    application::extensions::Contribution evaluate(const application::extensions::Input<Snapshot> &,
                                                   std::stop_token) override;
    void reset() noexcept override;
};
} // namespace mousebox::games::cs2::extensions
