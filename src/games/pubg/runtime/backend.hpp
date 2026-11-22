#pragma once
#include "runtime_models.hpp"
#include "control/output_mode.hpp"
#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include "domain/source_sample.hpp"
#include "application/ports/source_inspection.hpp"
#include <thread>

namespace mousebox
{
class SerialSession;
}
namespace mousebox::application
{
class OutputClient;
namespace ports
{
class OutputLink;
}
class Backend
{
    struct Data;
    class DeviceClient;
    std::unique_ptr<Data> data_;
    std::unique_ptr<SerialSession> device_;
    std::jthread livingWorker_, viewWorker_;
    std::atomic<bool> testArmed_{false};
    std::atomic<bool> sessionEligible_{false};
    std::atomic<bool> capturing_{false}, calibrateRequested_{false};
    std::mutex calibrationMutex_;
    control::OutputMode outputMode();
    void log(const std::string &);
    Snapshot observeSnapshot();
    OutputObservation outputObservationLocked() const;
    void boxOpened(unsigned baud, control::DeviceMotionLimits);
    void boxFailed(const std::string &);
    std::unique_ptr<OutputClient> makeDeviceClient();
    void calibrate(ports::OutputLink &, std::stop_token);
    void calibrationResult(const control::Calibration &, const std::string &, bool);
    void outputState(bool, bool, bool, control::MotionEvent, std::optional<control::Identity> held = {},
                     bool suspended = false, std::optional<control::MotionFeedback> feedback = {},
                     std::optional<std::uint64_t> observedGeneration = {});
    void buttonState(const control::ButtonObservation &, std::uint64_t, control::DeviceMotionLimits);
    void notifyScene();

  public:
    Backend();
    ~Backend();
    ports::SourcePackets inspectSources();
    struct PendingUpdates
    {
        bool calibrationFinished;
        std::vector<std::string> logs;
    };
    PendingUpdates takeUpdates();
    OutputObservation inspectOutput() const;
    PreviewObservation previewObservation();
    void readerDrawCompleted(std::span<const domain::SourceSample>, double completedAtMs);
    ReaderMetricsSnapshot readerMetricsSnapshot(std::uint64_t nowMs) const;
    void setSceneNotifier(std::function<void()>);
    Snapshot inspectSnapshot()
    {
        return observeSnapshot();
    }
    control::SpeedPreset speedPreset();
    MotionConfiguration motionConfiguration();
    bool outputObservationCurrent(std::uint64_t);
    void setHumanModel(std::shared_ptr<const control::HumanMotionModel>, unsigned dpi, bool variation);
    void setOutputProgram(std::shared_ptr<ports::OutputProgram>);
    void loadRecording(const std::filesystem::path &);
    void connectReader(const std::string &, unsigned short, double);
    void disconnectReader();
    void resetWorkspace();
    std::vector<Port> ports() const;
    void connectBox(const std::wstring &);
    void disconnectBox();
    void armTest(bool);
    void setCapturing(bool);
    void startCalibration(control::ViewMode = control::ViewMode::ThirdPerson,
                          const std::string &profile = "measured-source-view");
    void setCalibration(const control::Calibration &);
    void setProfiles(const std::vector<control::CalibrationRecord> &);
    void setOutputMode(std::optional<control::ViewMode>);
    void setInputMapping(control::InputMapping);
    void setGameInput(const control::GameInputSettings &);
    void setSpeed(control::SpeedPreset);
    void setTrajectory(const control::movement::TrajectorySettings &);
    void setTriggerButton(std::uint16_t);
    void setAimPart(control::AimPart);
    void setAimEnabled(bool);
    void setWeaponAimPolicy(const control::WeaponAimPolicy &);
    void setAimDistance(double meters);
    void setTargetPriority(control::TargetPriority);
    void setZeroingDistance(unsigned meters);
    void setGravity(double metersPerSecondSquared);
    void setBallisticCatalog(std::shared_ptr<const control::BallisticCatalog>);
    void setSourceSkew(double);
    void setLivingLocalSkew(double);
    void stopOutput();
    void setSessionEligible(bool);
    bool sessionEligible() const noexcept
    {
        return sessionEligible_.load();
    }
};
} // namespace mousebox::application
