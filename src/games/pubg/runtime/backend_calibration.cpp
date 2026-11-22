#include "backend_data.hpp"
#include "platform/windows/input.hpp"
#include <stdexcept>

namespace mousebox::application
{
void Backend::startCalibration(control::ViewMode mode, const std::string &profile)
{
    if (!sessionEligible_.load())
        throw std::runtime_error("运行会话未获资格");
    const auto current = observeSnapshot();
    if (current.calibrating)
        throw std::runtime_error("校准正在进行");
    if (current.capturing)
        throw std::runtime_error("请先暂停鼠标采集");
    if (!current.device.boxReady)
        throw std::runtime_error("请先连接鼠标盒子");
    testArmed_ = false;
    {
        const std::lock_guard lock(data_->mutex);
        auto &s = data_->snapshot;
        s.measurementMode = mode;
        s.measurementProfile = profile;
        s.calibrating = true;
    }
    calibrationResult({}, "等待当前 Reader 视点；请保持物理鼠标静止", false);
    calibrateRequested_ = true;
}

void Backend::calibrationResult(const control::Calibration &value, const std::string &status, bool finished)
{
    const std::lock_guard lock(data_->mutex);
    auto &current = data_->snapshot.calibration;
    if (current.measured != value.measured || current.yawDegreesPerCount != value.yawDegreesPerCount ||
        current.pitchDegreesPerCount != value.pitchDegreesPerCount || current.fov != value.fov)
        ++data_->snapshot.generation;
    auto &s = data_->snapshot;
    current = value;
    s.calibrationStatus = status;
    s.calibrationFinished = finished;
    s.calibrating = !finished;
    if (finished)
    {
        s.lastMeasurement = value;
        if (value.measured)
            data_->profiles.store({s.measurementMode, value});
        s.profiles = data_->profiles.records();
        data_->resolveCalibration();
    }
}

void Backend::setCalibration(const control::Calibration &value)
{
    setProfiles(value.measured ? std::vector<control::CalibrationRecord>{{control::ViewMode::ThirdPerson, value}}
                               : std::vector<control::CalibrationRecord>{});
}

void Backend::setProfiles(const std::vector<control::CalibrationRecord> &records)
{
    control::CalibrationProfiles next;
    for (const auto &record : records)
        next.store(record);
    stopOutput();
    const std::lock_guard lock(data_->mutex);
    auto &s = data_->snapshot;
    data_->profiles = std::move(next);
    s.profiles = data_->profiles.records();
    s.calibrating = false;
    s.calibrationFinished = false;
    s.lastMeasurement = {};
    ++s.motion.inputRevision;
    s.calibrationStatus = records.empty() ? "尚未校准" : "已载入多视角实测校准";
    data_->resolveCalibration();
}
} // namespace mousebox::application
