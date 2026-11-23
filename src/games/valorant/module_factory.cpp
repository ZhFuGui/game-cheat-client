#include "module_factory.hpp"
#include "definition.hpp"
#include "settings/configuration.hpp"
#include "diagnostics/source_state.hpp"
#include "radar/frame.hpp"
#include "presentation/submission.hpp"
#include <atomic>
#include <mutex>
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
    mutable std::mutex mutex_;
    VisualSettings visuals_;
    std::atomic<std::uint64_t> revision_ = 1;
    std::function<void()> notifier_;
    void changed()
    {
        ++revision_;
        std::function<void()> fn;
        {
            const std::lock_guard lock(mutex_);
            fn = notifier_;
        }
        if (fn)
            fn();
    }

  public:
    explicit Module(std::unique_ptr<application::ports::OutputBinding> output) : output_(std::move(output))
    {
        output_->bind(std::make_shared<ReadOnlyOutput>(), control::SpeedPreset::Normal, 8);
        source_.setNotifier([this] { changed(); });
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
    application::ports::PresentationFrame presentation(const application::ports::PresentationOptions &options) override
    {
        VisualSettings settings;
        {
            const std::lock_guard lock(mutex_);
            settings = visuals_;
        }
        return makeSubmission(source_.observe(), options, settings, revision_.load());
    }
    application::ports::GameWindowTarget presentationTarget() const override
    {
        const std::lock_guard lock(mutex_);
        return {visuals_.windowExecutable, 0};
    }
    void configure(const application::Json &values) override
    {
        for (auto item = values.begin(); item != values.end(); ++item)
            configuration().validateValue(item.key(), item.value());
        source_.setRates({values.at("hz").get<double>(), values.at("valorantSkeletonHz").get<double>(),
                          values.at("valorantViewHz").get<double>()});
        {
            const std::lock_guard lock(mutex_);
            visuals_ = visualSettings(values);
        }
        changed();
    }
    application::Json snapshot(bool) const override
    {
        const auto source = source_.observe();
        auto result = describeSource(source);
        const auto sequence = [&](unsigned index) {
            const auto &data = source.channels[index].data;
            return data ? std::to_string(data->capture.sequence) : std::string("—");
        };
        auto &state = result["state"];
        state["espSequences"] = "位置 #" + sequence(0) + " · 骨骼 #" + sequence(1) + " · 相机 #" + sequence(2);
        state["espFov"] = "原生相机重建（视口 / FOV 轴向可调整）";
        state["espUnits"] = "原生单位；米制比例由用户明确设置";
        return result;
    }
    std::optional<application::Json> command(std::string_view name, const application::Json &args) override
    {
        if (name == "game.valorant.scene.read")
            return readScene(source_.observe(), args);
        if (name == "game.valorant.radar.read")
        {
            VisualSettings settings;
            {
                const std::lock_guard lock(mutex_);
                settings = visuals_;
            }
            auto source = source_.observe();
            source.revision = revision_.load();
            return readRadar(source, args, settings);
        }
        return {};
    }
    void setHumanModel(std::shared_ptr<const control::HumanMotionModel>, unsigned, bool) override {}
    void setSceneNotifier(std::function<void()> fn) override
    {
        const std::lock_guard lock(mutex_);
        notifier_ = std::move(fn);
    }
    void drawCompleted(std::span<const domain::SourceSample>, double) override {}
};
} // namespace
std::unique_ptr<application::ports::GameModule> makeModule(std::unique_ptr<application::ports::OutputBinding> output)
{
    return std::make_unique<Module>(std::move(output));
}
} // namespace mousebox::games::valorant
