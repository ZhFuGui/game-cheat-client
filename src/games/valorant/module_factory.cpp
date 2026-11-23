#include "module_factory.hpp"
#include "definition.hpp"
#include "settings/configuration.hpp"
#include "diagnostics/source_state.hpp"
#include "diagnostics/aim_state.hpp"
#include "control/observation.hpp"
#include "control/settings.hpp"
#include "platform/windows/foreground_process.hpp"
#include "platform/windows/input.hpp"
#include "radar/frame.hpp"
#include "presentation/submission.hpp"
#include <atomic>
#include <mutex>
#include "adapters/sources/valorant_reader/client.hpp"

namespace mousebox::games::valorant
{
namespace
{
class Module final : public application::ports::GameModule, public application::ports::GameApplication
{
    std::unique_ptr<application::ports::OutputBinding> output_;
    sources::valorant_reader::Client source_;
    std::shared_ptr<aim::OutputProgram> program_;
    mutable aim::FrameCache controlFrames_;
    std::atomic<bool> eligible_ = false;
    std::atomic<bool> checkForeground_ = true;
    std::optional<aim::Settings> aimSettings_;
    unsigned trigger_ = 8;
    mutable std::mutex mutex_;
    VisualSettings visuals_;
    std::atomic<std::uint64_t> revision_ = 1;
    std::function<void()> notifier_;
    aim::Input controlInput() const
    {
        auto frame = controlFrames_.read(source_.observe());
        if (!frame || !checkForeground_.load())
            return {frame, false};
        std::string executable;
        {
            const std::lock_guard lock(mutex_);
            executable = visuals_.windowExecutable;
        }
        return {std::move(frame), foregroundExecutable(executable)};
    }
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
        program_ = std::make_shared<aim::OutputProgram>([this] { return controlInput(); });
        output_->bind(program_, control::SpeedPreset::Normal, 8);
        source_.setNotifier([this] {
            program_->setRequested(source_.requested());
            changed();
        });
    }
    ~Module()
    {
        source_.setNotifier({});
        output_->eligible(false);
        output_->stop();
        output_->bind({}, control::SpeedPreset::Normal, 8);
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
        eligible_ = false;
        output_->eligible(false);
        output_->stop();
        program_->revokeView();
        source_.disconnect();
    }
    void setSessionEligible(bool value) override
    {
        eligible_ = value;
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
        const auto settings = aim::readSettings(values);
        checkForeground_ = settings.checkForeground;
        const auto trigger = values.at("trigger").get<unsigned>();
        if (!aimSettings_ || *aimSettings_ != settings || trigger_ != trigger)
        {
            output_->eligible(false);
            output_->stop();
            program_->configure(settings);
            output_->bind(program_, settings.speed, static_cast<std::uint16_t>(trigger));
            aimSettings_ = settings;
            trigger_ = trigger;
            output_->eligible(eligible_.load());
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
        describeAim(state, program_->inspect(monotonicMilliseconds()));
        state["espSequences"] = "位置 #" + sequence(0) + " · 骨骼 #" + sequence(1) + " · 相机 #" + sequence(2);
        state["espFov"] = "原生相机重建（视口 / FOV 轴向可调整）";
        state["espUnits"] = "原生单位；米制比例由用户明确设置";
        return result;
    }
    std::optional<application::Json> command(std::string_view name, const application::Json &args) override
    {
        if (name == "game.valorant.aim.confirmView")
        {
            if (!args.is_object() || args.size() != 1 || !args.contains("confirmed") ||
                !args.at("confirmed").is_boolean())
                throw std::invalid_argument("视角确认必须为布尔值");
            output_->stop();
            const auto confirmed = program_->confirmView(args.at("confirmed").get<bool>(), monotonicMilliseconds());
            changed();
            return application::Json{{"confirmed", confirmed}};
        }
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
    void setHumanModel(std::shared_ptr<const control::HumanMotionModel> model, unsigned dpi, bool variation) override
    {
        program_->setHumanModel(std::move(model), dpi, variation);
    }
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
