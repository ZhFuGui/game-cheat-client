#include "runtime_service_impl.hpp"

namespace mousebox::application
{
ports::GameWindowTarget RuntimeService::presentationTarget() const
{
    const std::lock_guard lock(impl_->mutex);
    if (!impl_->workspaceAvailable || impl_->workspaceTransition || impl_->closed || !impl_->gameModule)
        return {};
    return impl_->gameModule->presentationTarget();
}
void RuntimeService::setPresentationActive(bool active)
{
    const std::lock_guard lock(impl_->mutex);
    impl_->presentationActive = active;
    if (impl_->gameModule)
        impl_->gameModule->setPresentationActive(active && impl_->workspaceAvailable && !impl_->workspaceTransition &&
                                                 !impl_->closed);
}
ports::PresentationFrame RuntimeService::presentation(const ports::PresentationOptions &options)
{
    const std::lock_guard lock(impl_->mutex);
    if (!impl_->workspaceAvailable || impl_->workspaceTransition || impl_->closed)
    {
        ports::PresentationFrame empty;
        empty.scene.palette = options.palette;
        empty.scene.english = options.english;
        return empty;
    }
    return impl_->gameModule->presentation(options);
}
} // namespace mousebox::application
