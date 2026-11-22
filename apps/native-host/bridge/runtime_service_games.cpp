#include "runtime_service_impl.hpp"
#include "runtime_service_util.hpp"
#include "composition/game_modules.hpp"

namespace mousebox::application
{
void RuntimeService::Impl::installModule(std::string_view id)
{
    auto next = options.createGameModule ? options.createGameModule(id, backend)
                                         : desktop::composition::createGameModule(id, backend);
    runtime_detail::require(next && next->definition().id == id, "工作区没有匹配的游戏模块");
    runtime_detail::require(id == "pubg" || next->application(), "此游戏缺少独立应用适配器");
    gameModule = std::move(next);
    gameModule->setPresentationActive(presentationActive);
    if (auto app = gameModule->application())
        app->setSceneNotifier(sceneNotifier);
    separateApplication.store(gameModule->application() != nullptr, std::memory_order_release);
}
void RuntimeService::Impl::configureActive()
{
    if (auto app = gameModule->application())
        app->configure(values());
    else
    {
        backend.setOutputProgram({});
        backend.setProfiles(calibrationRecords(settings["profiles"], calibrationSignature(values())));
        applySettings(backend, values());
    }
}
void RuntimeService::Impl::applyHumanModel(std::shared_ptr<const control::HumanMotionModel> model, unsigned dpi,
                                           bool variation)
{
    if (auto app = gameModule->application())
        app->setHumanModel(std::move(model), dpi, variation);
    else
        backend.setHumanModel(std::move(model), dpi, variation);
}
void RuntimeService::setSceneNotifier(std::function<void()> notify)
{
    const std::lock_guard lock(impl_->mutex);
    impl_->sceneNotifier = notify;
    impl_->backend.setSceneNotifier(notify);
    if (auto app = impl_->gameModule->application())
        app->setSceneNotifier(std::move(notify));
}
void RuntimeService::drawCompleted(std::span<const domain::SourceSample> samples, double completedAtMs)
{
    if (!impl_->separateApplication.load(std::memory_order_acquire))
    {
        impl_->backend.readerDrawCompleted(samples, completedAtMs);
        return;
    }
    const std::lock_guard lock(impl_->mutex);
    if (auto app = impl_->gameModule->application())
        app->drawCompleted(samples, completedAtMs);
    else
        impl_->backend.readerDrawCompleted(samples, completedAtMs);
}
} // namespace mousebox::application
