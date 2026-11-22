#include "module_factory.hpp"
#include "adapters/sources/cs2_reader/client.hpp"
#include "games/registry.hpp"
#include "presentation/extension_submission.hpp"
#include "presentation/radar_frame.hpp"
#include "extensions/frame_input.hpp"
#include "settings/configuration.hpp"
#include "diagnostics/runtime_state.hpp"
#include "diagnostics/controller_state.hpp"
#include "control/output_program.hpp"
#include "diagnostics/hitbox_state.hpp"
#include "platform/windows/input.hpp"
#include "platform/windows/foreground_process.hpp"
#include "hitboxes/assets.hpp"
#include "trigger/controller.hpp"
#include "rage/output_program.hpp"
#include <atomic>
#include <mutex>
namespace mousebox::games::cs2
{
namespace
{
using application::Json;
class Module final : public application::ports::GameModule, public application::ports::GameApplication
{
    sources::cs2_reader::Client source_;
    std::unique_ptr<application::ports::OutputBinding> output_;
    std::shared_ptr<OutputProgram> program_;
    std::shared_ptr<trigger::Controller> trigger_;
    std::shared_ptr<rage::OutputProgram> rage_;
    std::shared_ptr<trigger::OutputProgram> aimWithTrigger_;
    std::atomic<bool> eligible_ = false;
    mutable std::mutex mutex_;
    double scale_ = 0;
    hitboxes::Display hitboxDisplay_ = hitboxes::Display::Off;
    bool showHealth_ = true;
    bool checkForeground_ = true;
    bool aimCoverCheck_ = false;
    TargetFilter targetFilter_;
    Json configuredValues_;
    std::function<void()> notify_;
    std::atomic<std::uint64_t> revision_ = 1;
    std::atomic<bool> presentationActive_ = false;
    application::extensions::Host<Snapshot> extensions_;
    ControlInput observeControl() const
    {
        const auto o = source_.observe();
        return {o.frame, o.sample, o.revision, o.requested, o.current, o.observedAtMs, o.weaponAccuracyEvaluation};
    }
    void submitExtensions()
    {
        if (!presentationActive_)
            return;
        const auto o = source_.observe();
        extensions_.submit(
            extensions::input(o.current ? o.frame : nullptr, o.sample, o.revision, o.receivedAtMs, o.receipt));
    }
    void changed()
    {
        ++revision_;
        std::function<void()> fn;
        {
            std::lock_guard lock(mutex_);
            fn = notify_;
        }
        if (fn)
            fn();
    }

  public:
    explicit Module(std::unique_ptr<application::ports::OutputBinding> output,
                    std::vector<std::unique_ptr<application::extensions::Extension<Snapshot>>> extensions,
                    std::shared_ptr<application::ports::VisibilityProvider> geometry,
                    std::shared_ptr<application::ports::BallisticsProvider> ballistics)
        : output_(std::move(output)), extensions_("cs2", std::move(extensions))
    {
        const auto observe = [this] { return observeControl(); };
        program_ = std::make_shared<OutputProgram>(observe, OutputProgram::TargetSource{}, geometry);
        trigger_ = std::make_shared<trigger::Controller>(observe, foregroundProcessId, geometry);
        rage_ = std::make_shared<rage::OutputProgram>(observe, foregroundProcessId, std::move(geometry),
                                                      std::move(ballistics));
        aimWithTrigger_ = std::make_shared<trigger::OutputProgram>(program_, trigger_);
        source_.setNotifier([this] {
            program_->setRequested(source_.requested());
            rage_->setRequested(source_.requested());
            submitExtensions();
            changed();
        });
        extensions_.setNotifier([this] { changed(); });
    }
    ~Module()
    {
        source_.setNotifier({});
        eligible_ = false;
        output_->eligible(false);
        output_->stop();
        // Revoke physical output before waiting for any third-party analysis
        // or scene release. An optional visual extension cannot delay stopping.
        extensions_.shutdown();
        output_->bind({}, control::SpeedPreset::Normal, 8);
        source_.disconnect();
    }
    const GameDefinition &definition() const noexcept override
    {
        return *findGame("cs2");
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
        configuredValues_ = {};
        eligible_ = false;
        output_->eligible(false);
        source_.disconnect();
        extensions_.reset();
        output_->stop();
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
    void setPresentationActive(bool active) override
    {
        if (presentationActive_.exchange(active) == active)
            return;
        if (active)
            submitExtensions();
        else
            extensions_.reset();
    }
    application::ports::GameWindowTarget presentationTarget() const override
    {
        const auto observation = source_.observe();
        return {std::string(definition().windowExecutable), observation.frame ? observation.frame->processId : 0u};
    }
    application::ports::PresentationFrame presentation(const application::ports::PresentationOptions &o) override
    {
        auto observation = source_.observe();
        double scale;
        hitboxes::Display hitboxDisplay;
        auto options = o;
        {
            std::lock_guard lock(mutex_);
            scale = scale_;
            hitboxDisplay = hitboxDisplay_;
            options.esp.health = showHealth_;
        }
        return makeExtensionSubmission(
            extensions::input(observation.current ? observation.frame : nullptr, observation.sample,
                              observation.revision, observation.receivedAtMs, observation.receipt),
            extensions_, options, scale, revision_.load(), monotonicMilliseconds(), hitboxDisplay);
    }
    void configure(const Json &values) override
    {
        for (auto it = values.begin(); it != values.end(); ++it)
            configuration().validateValue(it.key(), it.value());
        // Triggerbot condition and timing edits keep the running device program.
        if (configuredValues_.is_object())
        {
            auto previous = configuredValues_;
            for (const auto key : {"cs2TriggerTimeOffsetMs", "cs2TriggerSniperInaccuracyLimit", "cs2TriggerCoverCheck",
                                   "cs2TriggerReactionEnabled", "cs2TriggerReactionMs", "cs2TriggerIntervalRangeMs",
                                   "cs2TriggerHoldRangeMs"})
                if (values.contains(key))
                    previous[key] = values.at(key);
            if (previous == values)
            {
                trigger_->configure(values);
                configuredValues_ = values;
                changed();
                return;
            }
        }
        if (values.value("cs2RageEnabled", false) && program_->inspect(monotonicMilliseconds(), false).calibrating)
            program_->cancelCalibration();
        if (values.value("cs2TriggerEnabled", false) && program_->inspect(monotonicMilliseconds(), false).calibrating)
            throw std::runtime_error("设备校准期间不能启用自动扳机");
        const bool aimEnabled = values.value("aimEnabled", true) && !values.value("cs2TriggerEnabled", false) &&
                                !values.value("cs2RageEnabled", false);
        const bool selectionChanged = program_->enabled() != aimEnabled ||
                                      trigger_->enabled() != values.value("cs2TriggerEnabled", false) ||
                                      rage_->enabled() != values.value("cs2RageEnabled", false);
        const bool checkForeground = values.value("cs2CheckForeground", true);
        const bool aimCoverCheck = values.value("cs2AimCoverCheck", false);
        auto settings = aimSettings(values);
        settings.hitboxAim = readHitboxAimSettings(values);
        const bool hitboxChanged = configuredValues_.is_object() &&
                                   readHitboxAimSettings(configuredValues_) != *settings.hitboxAim;
        const bool recoilChanged = configuredValues_.is_object() &&
                                   configuredValues_.value("cs2RecoilCompensation", true) != settings.recoil;
        if (checkForeground_ != checkForeground || aimCoverCheck_ != aimCoverCheck || targetFilter_ != settings.targets ||
            recoilChanged || hitboxChanged)
            output_->eligible(false);
        if (selectionChanged)
        {
            // Revoke in-flight output before disabling the old owner. The new
            // owner is configured only after all conflicting programs are off.
            output_->eligible(false);
            auto disabled = values;
            disabled["cs2TriggerEnabled"] = disabled["cs2RageEnabled"] = false;
            program_->setEnabled(false);
            trigger_->configure(disabled);
            rage_->configure(disabled);
        }
        source_.setMaximumAge(values.at("cs2MaxAgeMs").get<unsigned>());
        source_.setFrequency(values.at("hz").get<double>());
        program_->configure(settings);
        program_->setCoverCheck(aimCoverCheck);
        trigger_->configure(values);
        rage_->configure(values);
        program_->setEnabled(aimEnabled);
        checkForeground_ = checkForeground;
        aimCoverCheck_ = aimCoverCheck;
        targetFilter_ = settings.targets;
        extensions_.configure(values);
        const bool rageEnabled = rage_->enabled();
        output_->bind(rageEnabled ? std::static_pointer_cast<application::ports::OutputProgram>(rage_)
                                  : std::static_pointer_cast<application::ports::OutputProgram>(aimWithTrigger_),
                      rageEnabled ? rage::OutputProgram::MovementSpeed : settings.speed, settings.trigger);
        {
            std::lock_guard lock(mutex_);
            scale_ = values.at("cs2MetersPerUnit").get<double>();
            const auto mode = values.value("cs2HitboxDisplay", "off");
            hitboxDisplay_ = mode == "solid"       ? hitboxes::Display::Solid
                             : mode == "wireframe" ? hitboxes::Display::Wireframe
                                                   : hitboxes::Display::Off;
            showHealth_ = values.value("cs2ShowHealth", true);
        }
        output_->eligible(eligible_.load());
        configuredValues_ = values;
        submitExtensions();
        changed();
    }
    std::optional<Json> command(std::string_view name, const Json &args) override
    {
        if (name == "game.cs2.radar.read")
        {
            const auto current = source_.observe();
            return readRadarFrame(current.current ? current.frame.get() : nullptr, current.revision,
                                  current.metrics.connection, args);
        }
        if (name == "game.cs2.calibration.start")
        {
            const auto observation = program_->inspect(monotonicMilliseconds()).calibrationInput;
            if (trigger_->enabled())
                throw std::runtime_error("请先关闭自动扳机，再执行设备校准");
            if (rage_->enabled())
                throw std::runtime_error("请先关闭Rage，再执行设备校准");
            if (!args.empty() || !eligible_ || !observation.ready())
                throw std::runtime_error("CS2校准尚未就绪：" + observation.block);
            program_->startCalibration();
            return Json{{"ok", true}};
        }
        if (name == "game.cs2.calibration.cancel")
        {
            if (!args.empty())
                throw std::invalid_argument("校准取消不接受额外参数");
            program_->cancelCalibration();
            output_->stop();
            return Json{{"ok", true}};
        }
        return std::nullopt;
    }
    void setHumanModel(std::shared_ptr<const control::HumanMotionModel> model, unsigned dpi, bool variation) override
    {
        program_->setHumanModel(std::move(model), dpi, variation);
    }
    void setSceneNotifier(std::function<void()> fn) override
    {
        std::lock_guard lock(mutex_);
        notify_ = std::move(fn);
    }
    void drawCompleted(std::span<const domain::SourceSample> s, double at) override
    {
        source_.completed(s, at);
    }
    Json snapshot(bool diagnostics) const override
    {
        auto observation = source_.observe();
        double scale;
        hitboxes::Display hitboxDisplay;
        {
            std::lock_guard lock(mutex_);
            scale = scale_;
            hitboxDisplay = hitboxDisplay_;
        }
        auto result = describeSnapshot(observation, scale, diagnostics);
        result["state"]["cs2Hitboxes"] =
            describeHitboxes(observation.current ? observation.frame.get() : nullptr, hitboxDisplay);
        const auto now = monotonicMilliseconds();
        const auto rageSelected = rage_->enabled();
        describeController(result, program_->inspect(now, !rageSelected), eligible_.load(), diagnostics, !rageSelected);
        if (!program_->enabled())
        {
            result["state"]["canAim"] = false;
            result["state"]["aimArmed"] = false;
            result["state"]["cs2Controller"]["active"] = false;
            if (!rageSelected)
            {
                result["state"]["control"] = "Aimbot已关闭";
                result["state"]["cs2Controller"]["block"] = "disabled";
            }
        }
        result["state"]["cs2Trigger"] = trigger_->snapshot(eligible_.load());
        result["state"]["cs2Rage"] = rage_->snapshot(eligible_.load());
        auto extensionStates = extensions_.states(now);
        if (!presentationActive_)
            for (auto &state : extensionStates)
                if (state.enabled)
                {
                    state.phase = "paused";
                    state.code = "presentation-hidden";
                    state.message = "ESP不可见，扩展分析已暂停";
                }
        result["state"]["extensions"] = application::extensions::describe(extensionStates);
        return result;
    }
};
} // namespace
std::unique_ptr<application::ports::GameModule> makeModule(
    std::unique_ptr<application::ports::OutputBinding> output,
    std::vector<std::unique_ptr<application::extensions::Extension<Snapshot>>> extensions,
    std::shared_ptr<application::ports::VisibilityProvider> geometry,
    std::shared_ptr<application::ports::BallisticsProvider> ballistics)
{
    return std::make_unique<Module>(std::move(output), std::move(extensions), std::move(geometry),
                                    std::move(ballistics));
}
} // namespace mousebox::games::cs2
