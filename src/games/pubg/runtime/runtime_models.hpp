#pragma once
#include "games/pubg/model/control_frame.hpp"
#include "games/pubg/model/ballistics_settings.hpp"
#include "games/pubg/model/esp_layers.hpp"
#include "games/pubg/pipeline/live_controller.hpp"
#include "games/pubg/pipeline/calibration_profiles.hpp"
#include "control/button_feedback.hpp"
#include "application/ports/output_program.hpp"
#include "application/output/device_snapshot.hpp"
#include "games/pubg/diagnostics/reader_metrics.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mousebox::application
{
struct MotionConfiguration
{
    bool aimEnabled = true;
    control::SpeedPreset speed = control::SpeedPreset::Normal;
    control::movement::TrajectorySettings trajectory;
    std::shared_ptr<const control::HumanMotionModel> model;
    unsigned dpi = 1600;
    bool variation = false;
    std::uint16_t triggerButton = mousebox_buttons::Back;
    control::AimPart aimPart = control::AimPart::Head;
    control::WeaponAimPolicy weaponAim;
    double maximumAimDistanceMeters = control::DefaultMaximumAimDistanceMeters;
    control::TargetPriority targetPriority = control::TargetPriority::NearestDistance;
    unsigned zeroingDistanceMeters = control::DefaultZeroingDistanceMeters;
    double gravityMetersPerSecondSquared = control::DefaultGravityMetersPerSecondSquared;
    double livingLocalSkewMs = control::DefaultLivingLocalSkewMs;
    std::shared_ptr<const control::BallisticCatalog> ballisticCatalog;
    std::uint64_t inputRevision = 0;
    std::shared_ptr<ports::OutputProgram> externalProgram;
    bool operator==(const MotionConfiguration &) const = default;
};
struct Snapshot
{
    ReaderMetricsSnapshot readerMetrics;
    std::optional<control::Frame> living, view;
    bool livingConnected = false, viewConnected = false;
    bool readerRunning = false, aimArmed = false, calibrationFinished = false, calibrating = false;
    bool readerLivingConnecting = false, readerLocalConnecting = false;
    std::string readerLivingError, readerLocalError;
    DeviceSnapshot device;
    bool triggerDown = false, controlActive = false, releaseRequired = false;
    bool capturing = false;
    std::uint64_t livingObservedMs = 0, viewObservedMs = 0, generation = 1;
    std::uint64_t observationRevision = 1;
    control::Calibration calibration;
    control::Calibration lastMeasurement;
    control::CalibrationMatch calibrationMatch;
    std::vector<control::CalibrationRecord> profiles;
    std::optional<control::ViewMode> outputMode;
    control::InputMapping inputMapping = control::InputMapping::DynamicFov;
    control::GameInputSettings gameInput;
    std::optional<control::Identity> heldTarget;
    bool suspended = false;
    control::ViewMode measurementMode = control::ViewMode::ThirdPerson;
    std::string measurementProfile;
    control::SpeedPreset speed = control::SpeedPreset::Normal;
    MotionConfiguration motion;
    control::MotionEvent stoppedBy = control::MotionEvent::None;
    std::optional<control::MotionFeedback> motionFeedback;
    double sourceSkewMs = 800;
    std::string calibrationStatus = "尚未校准";
    std::optional<double> livingRtt, viewRtt;
    std::string source = "未连接";
};
struct Port
{
    std::wstring name, description;
};
struct PreviewObservation
{
    std::shared_ptr<const control::Frame> living, view;
    std::optional<control::Identity> heldTarget;
    control::AimPart part = control::AimPart::Head;
    double maximumDistanceMeters = control::DefaultMaximumAimDistanceMeters, sourceSkewMs = 800;
    std::uint64_t revision = 0, generation = 0;
    std::shared_ptr<const control::Frame> espLiving, espView;
    std::shared_ptr<const control::EspTeamInfo> espTeam;
    control::TargetPriority targetPriority = control::TargetPriority::NearestDistance;
    std::optional<unsigned> spectators;
    control::WeaponAimPolicy weaponAim;
    std::shared_ptr<const control::BallisticCatalog> ballisticCatalog;
    double livingLocalSkewMs = control::DefaultLivingLocalSkewMs;
};
} // namespace mousebox::application
