#include "backend_data.hpp"
#include "adapters/outputs/mousebox/serial_session.hpp"
#include "platform/windows/input.hpp"
#include <iterator>
#include <stdexcept>

namespace mousebox::application
{
Backend::Backend() : data_(std::make_unique<Data>()), device_(std::make_unique<SerialSession>()) {}

Backend::~Backend()
{
    disconnectBox();
    disconnectReader();
}

void Backend::setSceneNotifier(std::function<void()> notify)
{
    const std::lock_guard lock(data_->mutex);
    data_->sceneNotifier = std::move(notify);
}

void Backend::notifyScene()
{
    std::function<void()> notify;
    {
        const std::lock_guard lock(data_->mutex);
        notify = data_->sceneNotifier;
    }
    if (notify)
        notify();
}

PreviewObservation Backend::previewObservation()
{
    const std::lock_guard lock(data_->mutex);
    const auto &s = data_->snapshot;
    return {data_->previewLiving,
            data_->previewView,
            s.heldTarget,
            s.motion.aimPart,
            s.motion.maximumAimDistanceMeters,
            s.sourceSkewMs,
            s.observationRevision,
            s.generation,
            data_->espLayers.living(),
            data_->espLayers.view(),
            data_->espLayers.team(),
            s.motion.targetPriority,
            s.readerRunning && s.viewConnected && s.view ? s.view->spectatedCount : std::nullopt,
            s.motion.weaponAim,
            s.motion.ballisticCatalog,
            s.motion.livingLocalSkewMs};
}

void Backend::log(const std::string &message)
{
    const std::lock_guard lock(data_->mutex);
    data_->logs.push_back(message);
    if (data_->logs.size() > 250)
        data_->logs.pop_front();
}

void Backend::readerDrawCompleted(std::span<const domain::SourceSample> headers, double completedAtMs)
{
    data_->readerMetrics.completed(headers, completedAtMs);
}
ReaderMetricsSnapshot Backend::readerMetricsSnapshot(std::uint64_t nowMs) const
{
    return data_->readerMetrics.snapshot(nowMs);
}

OutputObservation Backend::outputObservationLocked() const
{
    const auto &s = data_->snapshot;
    OutputObservation result{s.device, s.motion.triggerButton, sessionEligible_.load(), false, s.calibrating,
                             capturing_.load()};
    result.device.testArmed = testArmed_.load();
    const bool requested = s.motion.externalProgram ? s.motion.externalProgram->requested()
                                                   : s.readerRunning && s.motion.aimEnabled;
    result.aimArmed = result.sessionEligible &&
                      control::outputMode(requested, result.device.boxReady, result.device.testArmed,
                                          result.calibrating, result.capturing) == control::OutputMode::AutomaticAim;
    return result;
}
OutputObservation Backend::inspectOutput() const
{
    const std::lock_guard lock(data_->mutex);
    return outputObservationLocked();
}

Snapshot Backend::observeSnapshot()
{
    const std::lock_guard lock(data_->mutex);
    const auto output = outputObservationLocked();
    auto result = data_->snapshot;
    result.device.testArmed = output.device.testArmed;
    result.capturing = output.capturing;
    result.aimArmed = output.aimArmed;
    return result;
}

control::OutputMode Backend::outputMode()
{
    if (!sessionEligible_.load())
        return control::OutputMode::Idle;
    const std::lock_guard lock(data_->mutex);
    const auto &s = data_->snapshot;
    return control::outputMode(
        s.motion.externalProgram ? s.motion.externalProgram->requested() : s.readerRunning && s.motion.aimEnabled,
        s.device.boxReady, testArmed_.load(), s.calibrating || calibrateRequested_.load(), capturing_.load());
}

control::SpeedPreset Backend::speedPreset()
{
    const std::lock_guard lock(data_->mutex);
    return data_->snapshot.speed;
}

MotionConfiguration Backend::motionConfiguration()
{
    const std::lock_guard lock(data_->mutex);
    return data_->snapshot.motion;
}

bool Backend::outputObservationCurrent(std::uint64_t revision)
{
    const std::lock_guard lock(data_->mutex);
    return data_->snapshot.observationRevision == revision;
}

Backend::PendingUpdates Backend::takeUpdates()
{
    const std::lock_guard lock(data_->mutex);
    PendingUpdates result{data_->snapshot.calibrationFinished, {}};
    result.logs.assign(std::make_move_iterator(data_->logs.begin()), std::make_move_iterator(data_->logs.end()));
    data_->logs.clear();
    return result;
}

void Backend::armTest(bool enabled)
{
    if (enabled)
    {
        const auto program = motionConfiguration().externalProgram;
        if (program && program->activation() && program->activation()->source == control::ActivationSource::Continuous)
            throw std::runtime_error("请先关闭持续自动控制，再启用桌面位移测试");
    }
    if (enabled && !sessionEligible_.load())
        throw std::runtime_error("运行会话未获资格");
    if (enabled)
        stopOutput();
    testArmed_ = enabled;
    log(enabled ? "F8 桌面测试已启用；先松开 F8，再按下。" : "F8 测试已停止；链路就绪后自动恢复侧键跟随。");
}

void Backend::setCapturing(bool active)
{
    if (capturing_.exchange(active) != active)
        stopOutput();
}

void Backend::outputState(bool down, bool active, bool release, control::MotionEvent reason,
                          std::optional<control::Identity> held, bool suspended,
                          std::optional<control::MotionFeedback> feedback,
                          std::optional<std::uint64_t> observedGeneration)
{
    bool changed = false;
    {
        const std::lock_guard lock(data_->mutex);
        auto &s = data_->snapshot;
        if (observedGeneration && *observedGeneration != s.generation)
            return;
        if (!sessionEligible_.load())
        {
            down = active = release = suspended = false;
            held.reset();
            feedback.reset();
        }
        changed = s.heldTarget != held;
        s.triggerDown = down;
        s.controlActive = active;
        s.releaseRequired = release;
        s.stoppedBy = reason;
        s.heldTarget = held;
        s.suspended = suspended;
        s.motionFeedback = active ? feedback : std::nullopt;
    }
    if (changed)
        notifyScene();
}

void Backend::resetWorkspace()
{
    // Revoke before joining workers. Device transport is global and stays open.
    sessionEligible_ = false;
    calibrateRequested_ = false;
    capturing_ = false;
    stopOutput();
    const std::lock_guard cancellation(calibrationMutex_);
    disconnectReader();
    {
        const std::lock_guard lock(data_->mutex);
        auto &s = data_->snapshot;
        data_->profiles.clear();
        data_->reader = {};
        s.profiles.clear();
        s.calibration = {};
        s.lastMeasurement = {};
        s.calibrationMatch = {};
        s.calibrating = s.calibrationFinished = false;
        s.calibrationStatus = "尚未校准";
        s.measurementProfile.clear();
        s.livingObservedMs = s.viewObservedMs = 0;
        s.heldTarget.reset();
        s.motionFeedback.reset();
        s.triggerDown = s.controlActive = s.releaseRequired = s.suspended = false;
        s.stoppedBy = control::MotionEvent::None;
        s.motion.model.reset();
        s.motion.ballisticCatalog.reset();
        s.motion.externalProgram.reset();
        ++s.generation;
        ++s.observationRevision;
        ++s.motion.inputRevision;
    }
    notifyScene();
}

void Backend::stopOutput()
{
    calibrateRequested_ = false;
    testArmed_ = false;
    std::shared_ptr<ports::OutputProgram> program;
    {
        const std::lock_guard lock(data_->mutex);
        auto &s = data_->snapshot;
        program = s.motion.externalProgram;
        ++s.generation;
        ++s.motion.inputRevision;
        if (s.calibrating)
        {
            s.calibrating = false;
            s.calibrationFinished = true;
            s.lastMeasurement = {};
            s.calibrationStatus = "校准已取消";
            data_->resolveCalibration();
        }
    }
    if (program)
        program->reset();
    outputState(false, false, false, control::MotionEvent::None);
    log("本次程序位移已停止；链路就绪后自动待命，松开侧键再按。");
}

void Backend::setSessionEligible(bool eligible)
{
    if (sessionEligible_.exchange(eligible) == eligible)
        return;
    stopOutput();
}
} // namespace mousebox::application
