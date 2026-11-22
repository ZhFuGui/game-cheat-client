#include "runtime_service_impl.hpp"
#include "runtime_service_util.hpp"
#include "games/pubg/runtime/status_text.hpp"
#include "application/settings/game_configurations.hpp"
#include "application/settings/application_values.hpp"

namespace mousebox::application
{
using namespace runtime_detail;

void RuntimeService::Impl::persist()
{
    require(workspaceAvailable || workspaceTransition, "失败工作区没有有效配置写入资格，请重新选择工作区");
    if (gameModule->application())
    {
        workspaceStore.save(workspaceStore.activeId(), settings, motionBinding());
        return;
    }
    const auto signature = calibrationSignature(values());
    auto profiles = Json::array();
    // Calibrations for other input configurations remain available after edits.
    for (const auto &record : settings["profiles"])
        if (!record.is_object() || !record.contains("profile") || !record["profile"].is_string() ||
            record["profile"] != signature)
            profiles.push_back(record);
    for (const auto &record : calibrationRecordsJson(backend.inspectSnapshot().profiles))
        profiles.push_back(record);
    settings["profiles"] = std::move(profiles);
    workspaceStore.save(workspaceStore.activeId(), settings, motionBinding());
}

std::optional<Json> RuntimeService::Impl::handleSettingsLocked(std::string_view command, const Json &payload)
{
    auto &d = *this;
    if (auto app = gameModule->application())
    {
        if (command == "settings.set")
        {
            const auto key = payload.at("key").get<std::string>();
            const auto value = payload.at("value");
            if (key == "dpi" || key == "renderAdapter")
                validateApplicationValue(key, value);
            else
                gameConfiguration(gameModule->definition().id).validateValue(key, value);
            const auto previous = values();
            if (previous.at(key) == value)
                return ok();
            if (key == "renderAdapter")
            {
                bool found = value == "auto";
                for (const auto &item : graphics)
                    found = found || item.at("value") == value;
                require(found, "请选择已检测到的显卡");
            }
            values() = gameConfiguration(gameModule->definition().id).updatedValues(previous, key, value);
            try
            {
                values() = gameConfiguration(gameModule->definition().id).normalize(settings).at("values");
                app->configure(values());
                persist();
            }
            catch (...)
            {
                values() = previous;
                app->configure(values());
                throw;
            }
            return ok();
        }
        if (command == "calibration.start" || command == "calibration.export" || command == "settings.import")
            throw std::runtime_error("当前游戏请使用其专用输入参数；不能载入其他游戏的校准配置");
    }
    if (command == "settings.set")
    {
        const auto key = payload.at("key").get<std::string>();
        const auto value = payload.at("value");
        validateSetting(key, value);
        if (d.values()[key] == value)
            return ok();
        if (key == "renderAdapter")
        {
            bool found = value == "auto";
            for (const auto &item : d.graphics)
                if (item["value"] == value)
                    found = true;
            require(found, "请选择已检测到的显卡");
        }
        auto next = d.values();
        next[key] = value;
        if (key == "mapping" && value == 0 && next["mode"] == -1)
            next["mode"] = 0;
        require(next["mapping"] != 0 || next["mode"] != -1, "实时FOV换算需要选择灵敏度模式");
        const bool input =
            key == "dpi" || key == "general" || key == "aim" || key == "ads" || key == "vertical" || key == "inverted";
        const auto old = d.settings;
        const auto modelWasActive = d.learningState.active;
        d.values() = next;
        try
        {
            if (input)
            {
                d.deactivate();
                d.backend.setCalibration({});
            }
            applySettings(d.backend, d.values());
            d.persist();
        }
        catch (...)
        {
            d.settings = old;
            applySettings(d.backend, d.values());
            if (input)
            {
                d.learningState.active = modelWasActive;
                d.applyHumanModel(modelWasActive ? d.learningState.model : nullptr, d.values()["dpi"].get<unsigned>(),
                                  modelWasActive && d.learningState.variation);
            }
            throw;
        }
        return ok();
    }
    if (command == "settings.import")
    {
        require(!d.backend.inspectSnapshot().readerRunning && !d.backend.inspectSnapshot().device.boxReady &&
                    !d.learningState.recording && !d.learningState.training,
                "请先断开连接再迁移设置");
        const auto imported = normalizeSettings(payload);
        const auto previous = d.settings;
        const auto modelWasActive = d.learningState.active;
        const auto captureDpi = d.learningState.captureDpi;
        d.deactivate();
        try
        {
            d.settings = imported;
            d.backend.setProfiles(calibrationRecords(d.settings["profiles"], calibrationSignature(d.values())));
            applySettings(d.backend, d.values());
            d.learningState.captureDpi = d.values()["dpi"].get<unsigned>();
            d.persist();
        }
        catch (...)
        {
            d.settings = previous;
            d.backend.setProfiles(calibrationRecords(d.settings["profiles"], calibrationSignature(d.values())));
            applySettings(d.backend, d.values());
            d.learningState.captureDpi = captureDpi;
            d.learningState.active = modelWasActive;
            d.applyHumanModel(modelWasActive ? d.learningState.model : nullptr, d.values()["dpi"].get<unsigned>(),
                              modelWasActive && d.learningState.variation);
            throw;
        }
        return ok();
    }
    if (command == "settings.save")
    {
        d.persist();
        return ok();
    }
    if (command == "esp.set")
    {
        const auto key = payload.at("key").get<std::string>();
        const auto value = payload.at("value");
        gameConfiguration(gameModule->definition().id).validateEspValue(key, value);
        const auto previous = d.settings["espValues"];
        d.settings["espValues"][key] = value;
        try
        {
            d.persist();
        }
        catch (...)
        {
            d.settings["espValues"] = previous;
            throw;
        }
        return ok();
    }
    if (command == "esp.reset")
    {
        const auto previous = d.settings["espValues"];
        d.settings["espValues"] = gameConfiguration(gameModule->definition().id).defaults().at("espValues");
        try
        {
            d.persist();
        }
        catch (...)
        {
            d.settings["espValues"] = previous;
            throw;
        }
        return ok();
    }
    if (command == "calibration.start")
    {
        const auto mode = payload.value("mode", d.values()["measurement"].get<int>());
        require(mode >= 0 && mode <= 3, "无效校准视角");
        d.calibrationSaved = false;
        d.backend.startCalibration(static_cast<control::ViewMode>(mode), calibrationSignature(d.values()));
        return ok();
    }
    if (command == "calibration.export")
    {
        const auto s = d.backend.inspectSnapshot();
        const auto &c = s.calibrationFinished ? s.lastMeasurement : s.calibration;
        Json doc = {
            {"version", 3},
            {"workspace", d.workspaceContext()},
            {"profiles", calibrationRecordsJson(s.profiles)},
            {"status", c.measured           ? "measured"
                       : c.formulaEstimated ? "formula_estimate"
                                            : "not_measured"},
            {"dpi", d.values()["dpi"]},
            {"general_sensitivity", d.values()["general"]},
            {"aim_sensitivity", d.values()["aim"]},
            {"ads_sensitivity", d.values()["ads"]},
            {"vertical_multiplier", d.values()["vertical"]},
            {"maximum_source_skew_ms", s.sourceSkewMs},
            {"maximum_aim_distance_meters", s.motion.maximumAimDistanceMeters},
            {"unlocked_target_priority", std::string(control::targetPriorityKey(s.motion.targetPriority))},
            {"inverted", d.values()["inverted"]},
            {"input_mapping", s.inputMapping == control::InputMapping::DynamicFov ? "dynamic_fov" : "measured_only"},
            {"sensitivity_mode", s.outputMode ? Json(modeName(*s.outputMode)) : Json{}},
            {"fov", c.fov},
            {"calibration_status", s.calibrationStatus},
            {"estimated_degrees_per_count",
             c.formulaEstimated ? Json{{"yaw", c.yawDegreesPerCount}, {"pitch", c.pitchDegreesPerCount}} : Json{}},
            {"measured_degrees_per_count",
             c.measured ? Json{{"yaw", c.yawDegreesPerCount}, {"pitch", c.pitchDegreesPerCount}} : Json{}}};
        require(storage::save(pathOf(payload), doc), "校准记录保存失败");
        return ok();
    }
    return std::nullopt;
}
} // namespace mousebox::application
