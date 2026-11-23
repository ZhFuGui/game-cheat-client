#include "game_modules.hpp"
#include "games/pubg/module_factory.hpp"
#include "games/pubg/runtime/backend.hpp"
#include "games/cs2/module_factory.hpp"
#include "games/valorant/module_factory.hpp"
#include "games/cs2/extensions/registry.hpp"
#include "adapters/spatial/traceline/provider.hpp"
#include "platform/windows/dynamic_library.hpp"
#include <stdexcept>

namespace mousebox::desktop::composition
{
namespace
{
class OutputBinding final : public application::ports::OutputBinding
{
    application::Backend &backend_;

  public:
    explicit OutputBinding(application::Backend &backend) : backend_(backend) {}
    void bind(std::shared_ptr<application::ports::OutputProgram> program, control::SpeedPreset speed,
              std::uint16_t trigger) override
    {
        backend_.setOutputProgram(std::move(program));
        backend_.setSpeed(speed);
        backend_.setTriggerButton(trigger);
    }
    void eligible(bool value) override
    {
        backend_.setSessionEligible(value);
    }
    void stop() override
    {
        backend_.stopOutput();
    }
};
} // namespace
std::unique_ptr<application::ports::GameModule> createGameModule(std::string_view id, application::Backend &backend)
{
    if (id == "pubg")
        return games::pubg::makeModule(backend);
    if (id == "valorant")
        return games::valorant::makeModule(std::make_unique<OutputBinding>(backend));
    if (id == "cs2")
    {
        auto geometry =
            std::make_shared<adapters::traceline::Provider>(platform::executableDirectory() / L"data/games/cs2/cover");
        return games::cs2::makeModule(std::make_unique<OutputBinding>(backend),
                                      games::cs2::extensions::create(geometry, geometry), geometry, geometry);
    }
    throw std::runtime_error("此游戏没有已装配运行模块");
}
} // namespace mousebox::desktop::composition
