#include "backend_data.hpp"
#include "platform/windows/input.hpp"
#include <stdexcept>
#include <cmath>

namespace mousebox::application
{
void Backend::Data::resolveCalibration()
{
    auto &s = snapshot;
    if (s.calibrating)
        return;
    const auto fov = s.view ? s.view->fov : std::nullopt;
    auto match = s.inputMapping == control::InputMapping::MeasuredOnly ? profiles.match(fov, s.outputMode)
                                                                       : control::CalibrationMatch{};
    const auto next =
        s.inputMapping == control::InputMapping::DynamicFov
            ? control::pubgInputResponse(fov, s.outputMode.value_or(control::ViewMode::ThirdPerson), s.gameInput)
                  .value_or(control::Calibration{})
        : match.record ? match.record->value
                       : control::Calibration{};
    // Dynamic response availability is a data condition; only explicit settings changes invalidate the held
    // intent.
    if (s.inputMapping == control::InputMapping::MeasuredOnly &&
        control::inputResponseContextChanged(s.calibration, next))
        ++s.generation;
    s.calibration = next;
    s.calibrationMatch = std::move(match);
}

void Backend::setHumanModel(std::shared_ptr<const control::HumanMotionModel> model, unsigned dpi, bool variation)
{
    if (model && (!model->valid() || dpi < 100 || dpi > 32000))
        throw std::invalid_argument("速度模型或输出DPI不合法");
    stopOutput();
    {
        const std::lock_guard lock(data_->mutex);
        auto &s = data_->snapshot;
        s.motion.model = std::move(model);
        s.motion.dpi = dpi;
        s.motion.variation = variation;
        ++s.generation;
    }
    log(motionConfiguration().model ? "已采用学习到的速度和起停上限；链路就绪后自动待命，松开侧键再按。"
                                    : "已移除速度模型，恢复默认速度参数；链路就绪后自动待命。");
}

void Backend::setBallisticCatalog(std::shared_ptr<const control::BallisticCatalog> catalog)
{
    const std::lock_guard lock(data_->mutex);
    auto &s = data_->snapshot;
    if (s.motion.ballisticCatalog == catalog)
        return;
    s.motion.ballisticCatalog = std::move(catalog);
    ++s.generation;
    ++s.motion.inputRevision;
}
void Backend::setOutputProgram(std::shared_ptr<ports::OutputProgram> program)
{
    if (program && program->activation() && program->activation()->source == control::ActivationSource::Continuous)
        testArmed_ = false;
    std::shared_ptr<ports::OutputProgram> previous;
    {
        const std::lock_guard lock(data_->mutex);
        auto &s = data_->snapshot;
        if (s.motion.externalProgram == program)
            return;
        previous = std::move(s.motion.externalProgram);
        s.motion.externalProgram = std::move(program);
        ++s.generation;
        ++s.motion.inputRevision;
    }
    if (previous)
        previous->reset();
}

void Backend::setTriggerButton(std::uint16_t mask)
{
    if (mask != mousebox_buttons::Back && mask != mousebox_buttons::Forward)
        throw std::invalid_argument("无效的鼠标侧键");
    stopOutput();
    const std::lock_guard lock(data_->mutex);
    data_->snapshot.motion.triggerButton = mask;
    ++data_->snapshot.generation;
}
void Backend::setAimEnabled(bool enabled)
{
    const std::lock_guard lock(data_->mutex);
    auto &s = data_->snapshot;
    if (s.motion.aimEnabled == enabled)
        return;
    s.motion.aimEnabled = enabled;
    s.heldTarget.reset();
    s.controlActive = s.suspended = false;
    s.motionFeedback.reset();
    ++s.generation;
    ++s.motion.inputRevision;
}

void Backend::setOutputMode(std::optional<control::ViewMode> mode)
{
    const std::lock_guard lock(data_->mutex);
    auto &s = data_->snapshot;
    if (mode && (*mode < control::ViewMode::ThirdPerson || *mode > control::ViewMode::Ads))
        throw std::invalid_argument("无效的灵敏度模式");
    if (s.outputMode != mode)
    {
        s.outputMode = mode;
        ++s.generation;
        ++s.motion.inputRevision;
        data_->resolveCalibration();
    }
}

void Backend::setInputMapping(control::InputMapping mapping)
{
    if (mapping != control::InputMapping::DynamicFov && mapping != control::InputMapping::MeasuredOnly)
        throw std::invalid_argument("无效的输入换算方式");
    stopOutput();
    const std::lock_guard lock(data_->mutex);
    auto &s = data_->snapshot;
    if (s.inputMapping != mapping)
    {
        s.inputMapping = mapping;
        ++s.generation;
        ++s.motion.inputRevision;
        data_->resolveCalibration();
    }
}

void Backend::setGameInput(const control::GameInputSettings &input)
{
    if (!control::validGameInputSettings(input))
        throw std::invalid_argument("游戏灵敏度参数不合法");
    stopOutput();
    const std::lock_guard lock(data_->mutex);
    auto &s = data_->snapshot;
    if (s.gameInput != input)
    {
        s.gameInput = input;
        ++s.generation;
        ++s.motion.inputRevision;
        data_->resolveCalibration();
    }
}

void Backend::setSpeed(control::SpeedPreset speed)
{
    control::speedPolicy(speed);
    const std::lock_guard lock(data_->mutex);
    if (data_->snapshot.speed != speed)
    {
        data_->snapshot.speed = speed;
        data_->snapshot.motion.speed = speed;
        ++data_->snapshot.generation;
    }
}

void Backend::setTrajectory(const control::movement::TrajectorySettings &trajectory)
{
    if (!trajectory.valid())
        throw std::invalid_argument("无效鼠标轨迹设置");
    const std::lock_guard lock(data_->mutex);
    if (data_->snapshot.motion.trajectory != trajectory)
    {
        data_->snapshot.motion.trajectory = trajectory;
        ++data_->snapshot.generation;
    }
}

void Backend::setAimPart(control::AimPart part)
{
    if (control::aimBoneName(part).empty())
        throw std::invalid_argument("无效的瞄准部位");
    const std::lock_guard lock(data_->mutex);
    auto &s = data_->snapshot;
    if (s.motion.aimPart != part)
    {
        s.motion.aimPart = part;
        ++s.generation;
    }
}

void Backend::setWeaponAimPolicy(const control::WeaponAimPolicy &policy)
{
    if (!control::validWeaponAimPolicy(policy))
        throw std::invalid_argument("无效的枪械分类瞄准部位");
    const std::lock_guard lock(data_->mutex);
    auto &s = data_->snapshot;
    if (s.motion.weaponAim != policy)
    {
        s.motion.weaponAim = policy;
        ++s.generation;
    }
}

void Backend::setAimDistance(double meters)
{
    if (!control::validAimDistance(meters))
        throw std::invalid_argument("无效的自瞄距离上限");
    const std::lock_guard lock(data_->mutex);
    auto &s = data_->snapshot;
    if (s.motion.maximumAimDistanceMeters != meters)
    {
        s.motion.maximumAimDistanceMeters = meters;
        ++s.generation;
    }
}

void Backend::setTargetPriority(control::TargetPriority priority)
{
    if (!control::validTargetPriority(priority))
        throw std::invalid_argument("无效的目标优先级");
    const std::lock_guard lock(data_->mutex);
    auto &s = data_->snapshot;
    if (s.motion.targetPriority != priority)
    {
        s.motion.targetPriority = priority;
        ++s.generation;
    }
}

void Backend::setZeroingDistance(unsigned meters)
{
    if (!control::validZeroingDistance(meters))
        throw std::invalid_argument("无效的归零距离档位");
    const std::lock_guard lock(data_->mutex);
    auto &s = data_->snapshot;
    if (s.motion.zeroingDistanceMeters != meters)
    {
        s.motion.zeroingDistanceMeters = meters;
        ++s.generation;
        ++s.motion.inputRevision;
    }
}

void Backend::setGravity(double metersPerSecondSquared)
{
    if (!control::validGravity(metersPerSecondSquared))
        throw std::invalid_argument("重力须为0至100 m/s²的有限数值");
    const std::lock_guard lock(data_->mutex);
    auto &s = data_->snapshot;
    if (s.motion.gravityMetersPerSecondSquared != metersPerSecondSquared)
    {
        s.motion.gravityMetersPerSecondSquared = metersPerSecondSquared;
        ++s.generation;
        ++s.motion.inputRevision;
    }
}

void Backend::setSourceSkew(double value)
{
    if (!std::isfinite(value) || value < 0 || value > 5000)
        throw std::invalid_argument("无效的采样时间差");
    const std::lock_guard lock(data_->mutex);
    auto &s = data_->snapshot;
    if (s.sourceSkewMs != value)
    {
        s.sourceSkewMs = value;
        ++s.generation;
        ++s.motion.inputRevision;
    }
}

void Backend::setLivingLocalSkew(double value)
{
    if (!control::validLivingLocalSkew(value))
        throw std::invalid_argument("Living / Local补偿时间差须为0至5000毫秒的整数");
    const std::lock_guard lock(data_->mutex);
    auto &s = data_->snapshot;
    if (s.motion.livingLocalSkewMs != value)
    {
        s.motion.livingLocalSkewMs = value;
        ++s.generation;
        ++s.motion.inputRevision;
    }
}
} // namespace mousebox::application
