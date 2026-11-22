#include "output_loop.hpp"
#include <utility>

namespace mousebox::application
{
using control::OutputMode;

OutputLoop::OutputLoop(ports::OutputLink &link, OutputClient &client, OutputEnvironment environment,
                       std::uint32_t session, std::stop_token stop)
    : link_(link), client_(client), environment_(std::move(environment)), stop_(stop), selection_(client.select()),
      limits_(link.motionLimits()), session_(session)
{
    selection_.mode = OutputMode::Idle;
    selection_.activation.reset();
}

std::uint32_t OutputLoop::nextSession()
{
    if (!++session_)
        ++session_;
    return session_;
}

void OutputLoop::gate(bool enabled)
{
    const auto activation = selection_.activation.value_or(
        control::ActivationBinding{control::ActivationSource::DeviceMouse, selection_.trigger});
    link_.gate(activation.sideButton() ? activation.code : selection_.trigger, enabled && activation.sideButton());
}

void OutputLoop::desktopTest(std::uint64_t now)
{
    auto action = test_.poll(environment_.keyDown(environment_.testKey), now);
    if (action == HoldAction::Begin)
    {
        nextSession();
        sequence_ = 1;
        link_.beginPaced(session_, 32);
    }
    if (action == HoldAction::Begin || action == HoldAction::Refresh)
    {
        if (client_.testArmed() && environment_.keyDown(environment_.testKey) && !stop_.stop_requested())
            link_.refreshPaced(session_, ++sequence_, 1, 0, 10, 80);
        else
            action = HoldAction::Stop;
    }
    if (action == HoldAction::Stop)
        link_.stop(0);
}

void OutputLoop::automatic(const control::ButtonObservation &buttons)
{
    const auto now = environment_.now();
    const auto policy = control::speedPolicy(selection_.speed, limits_);
    const bool fresh = buttons.supportsMotion(policy.maximumStepCounts, policy.intervalMs, now);
    const auto generation = client_.generation();
    auto &program = *selection_.program;
    const auto activation = selection_.activation.value_or(
        control::ActivationBinding{control::ActivationSource::DeviceMouse, selection_.trigger});
    const auto isDown = [&](const control::ButtonObservation &observation) {
        return control::activationDown(activation, observation,
                                       activation.source == control::ActivationSource::LocalKeyboard &&
                                           environment_.keyDown(static_cast<int>(activation.code)));
    };
    const auto released = control::activationRelease(activation, buttons);
    const bool down = fresh && isDown(buttons);
    const auto result = program.tick({now, generation, fresh, down, limits_, fresh ? released : std::nullopt});
    if (result.motion.stop)
        link_.stop(0);
    if (result.motion.dx || result.motion.dy)
    {
        const auto latest = link_.buttons();
        if (!client_.current() || link_.motionLimits() != limits_ || stop_.stop_requested() ||
            !latest.supportsMotion(policy.maximumStepCounts, policy.intervalMs, environment_.now()) ||
            !isDown(latest) || control::activationRelease(activation, latest) != released ||
            program.activation() != selection_.activation ||
            !program.current(result.sourceRevision, environment_.now()) ||
            !link_.finiteCorrection(nextSession(), result.motion.dx, result.motion.dy))
        {
            program.reject(result);
            link_.stop(0);
        }
    }
    client_.motion(down, result, generation);
    const auto pulseButtons = link_.buttons();
    const auto click = program.buttonPulse({environment_.now(), generation, pulseButtons.fresh(environment_.now()),
                                            link_.supportsButtonPulses(), pulseButtons.down(1)});
    if (click)
    {
        const auto latest = link_.buttons();
        const bool accepted = client_.current() && !stop_.stop_requested() && latest.fresh(environment_.now()) &&
                              !latest.down(1) && program.buttonPulseCurrent(*click, environment_.now()) &&
                              link_.finiteLeftClick(nextSession(), click->holdMs);
        program.buttonPulseAccepted(accepted);
    }
}

unsigned OutputLoop::tick()
{
    link_.check();
    if (client_.measurementPending())
    {
        link_.stop(0);
        gate(false);
        client_.measure(link_, stop_);
        selection_.mode = OutputMode::Idle;
    }
    const auto next = client_.select();
    const auto limits = link_.motionLimits();
    if (next != selection_ || limits != limits_)
    {
        link_.stop(0);
        if (selection_.program)
            selection_.program->reset();
        test_ = HoldPlan(320);
        selection_ = next;
        limits_ = limits;
        client_.transitioned(limits_);
        gate(selection_.mode == OutputMode::AutomaticAim);
        client_.motion(false, {}, std::nullopt);
    }
    const auto buttons = link_.buttons();
    // Observe first, then read the comparison clock: feedback can arrive at any time.
    const auto now = environment_.now();
    client_.buttons(buttons, now, limits_);
    switch (selection_.mode)
    {
    case OutputMode::DesktopTest:
        desktopTest(now);
        break;
    case OutputMode::AutomaticAim:
        if (selection_.program)
            automatic(buttons);
        else
            client_.legacyTick(link_, buttons, limits_, [this] { return nextSession(); }, stop_);
        return 1;
    case OutputMode::Idle:
        break;
    }
    return 2;
}
} // namespace mousebox::application
