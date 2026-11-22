#include "ballistics_session.hpp"
#include <stdexcept>

namespace mousebox::games::cs2::extensions
{
BallisticsSession::BallisticsSession(std::shared_ptr<application::ports::BallisticsProvider> provider)
    : provider_(std::move(provider))
{
    if (!provider_)
        throw std::invalid_argument("Ballistics analysis needs an explicit provider");
}
bool BallisticsSession::available() const noexcept
{
    return provider_->available();
}
void BallisticsSession::retainScope(const application::extensions::Scope &scope, unsigned build) noexcept
{
    if (scope_ && (*scope_ != scope || build_ != build))
        reset();
}
std::shared_ptr<const application::ports::BallisticsScene> BallisticsSession::acquire(
    const application::extensions::Scope &scope, const MapIdentity &map, unsigned build, std::uint64_t observedAtMs)
{
    retainScope(scope, build);
    if (scene_)
        return scene_;
    scope_ = scope;
    build_ = build;
    if (retryAtMs_ == 0 || observedAtMs >= retryAtMs_)
        try
        {
            scene_ = provider_->loadBallistics(map.name, map.resourcePath, build);
            error_.clear();
        }
        catch (const std::exception &e)
        {
            error_ = e.what();
            retryAtMs_ = observedAtMs + 5000;
        }
    return scene_;
}
const std::string &BallisticsSession::error() const noexcept
{
    return error_;
}
void BallisticsSession::reset() noexcept
{
    scene_.reset();
    scope_.reset();
    build_ = 0;
    retryAtMs_ = 0;
    error_.clear();
}
} // namespace mousebox::games::cs2::extensions
