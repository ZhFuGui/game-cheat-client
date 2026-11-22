#pragma once
#include "application/ports/scene_visibility.hpp"
#include "application/ports/shot_prediction.hpp"
#include <filesystem>

namespace mousebox::adapters::traceline
{
// Lazy I/O and DLL initialization occur only on the extension worker.
class Provider final : public application::ports::VisibilityProvider, public application::ports::BallisticsProvider
{
    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    explicit Provider(std::filesystem::path root);
    ~Provider();
    bool available() const noexcept override;
    std::shared_ptr<const application::ports::VisibilityScene> load(std::string_view map,
                                                                    std::string_view resource) override;
    std::shared_ptr<const application::ports::BallisticsScene> loadBallistics(std::string_view map,
                                                                              std::string_view resource,
                                                                              unsigned gameBuild) override;
};
} // namespace mousebox::adapters::traceline
