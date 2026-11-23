#include "module_factory.hpp"
#include "definition.hpp"
#include "settings/configuration.hpp"
#include "diagnostics/source_state.hpp"
#include "radar/frame.hpp"
#include "adapters/sources/valorant_reader/client.hpp"

namespace mousebox::games::valorant
{
namespace
{
class ReadOnlyOutput final : public application::ports::OutputProgram
{
  public:
    bool requested() const noexcept override
    {
        return false;
    }
    application::ports::ProgramStep tick(const application::ports::OutputTick &) override
    {
        return {};
    }
    bool current(std::uint64_t, std::uint64_t) const override
    {
        return false;
    }
    void reject(const application::ports::ProgramStep &) override {}
    void reset() override {}
};
class Module final : public application::ports::GameModule, public application::ports::GameApplication
{
    std::unique_ptr<application::ports::OutputBinding> output_;
    sources::valorant_reader::Client source_;

  public:
    explicit Module(std::unique_ptr<application::ports::OutputBinding> output) : output_(std::move(output))
    {
        output_->bind(std::make_shared<ReadOnlyOutput>(), control::SpeedPreset::Normal, 8);
    }
    ~Module()
    {
        source_.setNotifier({});
        output_->eligible(false);
        output_->stop();
        source_.disconnect();
    }
    const GameDefinition &definition() const noexcept override
    {
        return Definition;
    }
    application::ports::DataSource &dataSource() noexcept override
    {
        return source_;
    }
    application::ports::GameApplication *application() noexcept override
    {
        return this;
    }
    void resetWorkspace() override
    {
        output_->eligible(false);
        output_->stop();
        source_.disconnect();
    }
    void setSessionEligible(bool value) override
    {
        output_->eligible(value);
    }
    void stopOutput() override
    {
        output_->stop();
    }
    application::ports::PresentationFrame presentation(const application::ports::PresentationOptions &) override
    {
        return {};
    }
    void configure(const application::Json &values) override
    {
        for (auto item = values.begin(); item != values.end(); ++item)
            configuration().validateValue(item.key(), item.value());
        source_.setRates({values.at("hz").get<double>(), values.at("valorantSkeletonHz").get<double>(),
                          values.at("valorantViewHz").get<double>()});
    }
    application::Json snapshot(bool) const override
    {
        return describeSource(source_.observe());
    }
    std::optional<application::Json> command(std::string_view name, const application::Json &args) override
    {
        if (name == "game.valorant.scene.read")
            return readScene(source_.observe(), args);
        if (name == "game.valorant.radar.read")
            return readRadar(source_.observe(), args);
        return {};
    }
    void setHumanModel(std::shared_ptr<const control::HumanMotionModel>, unsigned, bool) override {}
    void setSceneNotifier(std::function<void()> fn) override
    {
        source_.setNotifier(std::move(fn));
    }
    void drawCompleted(std::span<const domain::SourceSample>, double) override {}
};
} // namespace
std::unique_ptr<application::ports::GameModule> makeModule(std::unique_ptr<application::ports::OutputBinding> output)
{
    return std::make_unique<Module>(std::move(output));
}
} // namespace mousebox::games::valorant
