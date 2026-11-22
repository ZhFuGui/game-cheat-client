#include "module_factory.hpp"
#include "runtime/backend.hpp"
#include "games/registry.hpp"
#include "presentation/submission.hpp"
#include "pipeline/target_selector.hpp"
#include "platform/windows/input.hpp"

namespace mousebox::games::pubg
{
namespace
{
class ReaderSource final : public application::ports::DataSource
{
    Runtime &runtime_;

  public:
    explicit ReaderSource(Runtime &runtime) : runtime_(runtime) {}
    void connect(const application::ports::SourceConnection &connection) override
    {
        runtime_.connectReader(connection.host, connection.port, connection.hz);
    }
    void disconnect() override
    {
        runtime_.disconnectReader();
    }
    bool requested() const override
    {
        return runtime_.inspectSnapshot().readerRunning;
    }
    application::ports::SourcePackets inspectSources() const override
    {
        return runtime_.inspectSources();
    }
};
class Module final : public application::ports::GameModule
{
    Runtime &runtime_;
    ReaderSource source_;

  public:
    explicit Module(Runtime &runtime) : runtime_(runtime), source_(runtime) {}
    const GameDefinition &definition() const noexcept override
    {
        return *findGame("pubg");
    }
    application::ports::DataSource &dataSource() noexcept override
    {
        return source_;
    }
    void resetWorkspace() override
    {
        runtime_.resetWorkspace();
    }
    void setSessionEligible(bool eligible) override
    {
        runtime_.setSessionEligible(eligible);
    }
    void stopOutput() override
    {
        runtime_.stopOutput();
    }
    application::ports::PresentationFrame presentation(const application::ports::PresentationOptions &options) override
    {
        const auto s = runtime_.previewObservation();
        std::optional<control::Target> target;
        if (s.living && s.view)
        {
            control::TargetSelector selector;
            control::TargetPolicy policy;
            policy.part = control::resolveWeaponAim(*s.living, s.ballisticCatalog.get(), s.part, s.weaponAim).part;
            policy.priority = s.targetPriority;
            policy.livingLocalSkewMs = s.livingLocalSkewMs;
            policy.maximumDistanceMeters = s.maximumDistanceMeters;
            policy.requiredIdentity = s.heldTarget;
            target = selector.select(*s.living, *s.view, control::checkPair(*s.living, *s.view, {s.sourceSkewMs, 100}),
                                     policy, monotonicMilliseconds());
        }
        render::Scene scene{s.espLiving, s.espView, target, false, s.revision, options.palette, options.english};
        scene.esp = options.esp;
        scene.espTeam = s.espTeam;
        scene.heldTarget = s.heldTarget;
        scene.aimConeDegrees = control::TargetPolicy{}.maximumConeDegrees;
        scene.spectators = s.spectators;
        return makeSubmission(std::move(scene));
    }
};
} // namespace
std::unique_ptr<application::ports::GameModule> makeModule(Runtime &runtime)
{
    return std::make_unique<Module>(runtime);
}
} // namespace mousebox::games::pubg
