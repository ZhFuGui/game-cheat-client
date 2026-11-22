#include "runtime_service_impl.hpp"
#include "runtime_service_util.hpp"
#include "application/storage/human_store.hpp"

namespace mousebox::application
{
using namespace runtime_detail;
std::optional<Json> RuntimeService::Impl::handleModelsLocked(std::string_view command, const Json &payload)
{
    auto &d = *this;
    if (command == "model.import")
    {
        require(!d.learningState.training && !d.learningState.recording, "请先暂停采集或拟合");
        const auto path = pathOf(payload);
        d.ioBusy = true;
        d.learningState.status = "正在载入模型…";
        d.enqueue([&d, path] {
            try
            {
                const auto kind = humanModelFileKind(path);
                require(kind != ModelFileKind::Unknown, "文件不是受支持的模型");
                auto model = kind == ModelFileKind::Speed
                                 ? std::make_shared<const control::HumanMotionModel>(loadHumanModel(path))
                                 : nullptr;
                const auto canonical = std::filesystem::canonical(path);
                const std::lock_guard done(d.mutex);
                d.deactivate();
                d.invalidateDeletion();
                d.learningState.model = std::move(model);
                d.learningState.modelPath = canonical;
                d.learningState.status = "模型已载入，尚未启用";
                d.persist();
                d.finishIo();
            }
            catch (const std::exception &e)
            {
                const std::lock_guard done(d.mutex);
                d.finishIo(e.what());
            }
        });
        return ok();
    }
    if (command == "model.export")
    {
        require(d.learningState.model && !d.learningState.training && !d.learningState.recording,
                "没有可导出的速度模型");
        const auto path = pathOf(payload);
        const auto model = d.learningState.model;
        d.ioBusy = true;
        d.learningState.status = "正在导出模型…";
        d.enqueue([&d, path, model] {
            try
            {
                require(saveHumanModel(path, *model), "模型保存失败");
                const auto canonical = std::filesystem::canonical(path);
                const std::lock_guard done(d.mutex);
                d.invalidateDeletion();
                d.learningState.modelPath = canonical;
                d.learningState.status = "模型已导出，此前副本保留";
                d.persist();
                d.finishIo();
            }
            catch (const std::exception &e)
            {
                const std::lock_guard done(d.mutex);
                d.finishIo(e.what());
            }
        });
        return ok();
    }
    if (command == "model.setActive")
    {
        require(!d.learningState.training && !d.learningState.recording, "请先暂停采集或拟合");
        const bool active = payload.at("active").get<bool>();
        require(!active || static_cast<bool>(d.learningState.model), "没有可用的速度模型");
        if (active != d.learningState.active)
        {
            d.applyHumanModel(active ? d.learningState.model : nullptr, d.values()["dpi"].get<unsigned>(),
                              active && d.learningState.variation);
            d.learningState.active = active;
            d.persist();
        }
        return ok();
    }
    if (command == "model.requestDelete")
    {
        require(!d.learningState.training && !d.learningState.recording, "请先暂停采集或拟合");
        require(d.learningState.model || !d.learningState.modelPath.empty(), "没有已载入模型");
        d.learningState.deletePath = d.learningState.modelPath;
        d.learningState.deleteDigest.clear();
        if (!d.learningState.deletePath.empty() && std::filesystem::exists(d.learningState.deletePath))
        {
            require(humanModelFileKind(d.learningState.deletePath) != ModelFileKind::Unknown,
                    "这是采样或未知文件，不会删除");
            d.learningState.deletePath = std::filesystem::canonical(d.learningState.deletePath);
            d.learningState.modelPath = d.learningState.deletePath;
            d.learningState.deleteDigest = storage::sha256(storage::readBytes(d.learningState.deletePath, 1024 * 1024));
        }
        d.learningState.deleteGeneration = d.learningState.modelGeneration;
        d.learningState.deleteToken = unique();
        return Json{{"token", d.learningState.deleteToken},
                    {"path", storage::utf8(d.learningState.deletePath.wstring())}};
    }
    if (command == "model.confirmDelete")
    {
        require(!d.learningState.training && !d.learningState.recording && !d.learningState.deleteToken.empty() &&
                    payload.at("token") == d.learningState.deleteToken &&
                    d.learningState.deleteGeneration == d.learningState.modelGeneration &&
                    d.learningState.modelPath == d.learningState.deletePath,
                "删除确认已失效");
        if (!d.learningState.deletePath.empty() && std::filesystem::exists(d.learningState.deletePath))
        {
            const auto references =
                d.workspaceStore.modelReferences(d.learningState.deletePath, d.workspaceStore.activeId());
            if (!references.empty())
            {
                std::string names;
                for (const auto &name : references)
                    names += (names.empty() ? "" : "、") + name;
                throw std::runtime_error("模型仍被其他游戏引用：" + names + "；原文件和绑定均保留");
            }
            require(std::filesystem::canonical(d.learningState.deletePath) == d.learningState.deletePath &&
                        storage::sha256(storage::readBytes(d.learningState.deletePath, 1024 * 1024)) ==
                            d.learningState.deleteDigest,
                    "模型文件已改变，请重新确认删除");
            d.deactivate();
            require(removeHumanModel(d.learningState.deletePath), "模型停用，但文件删除失败");
        }
        else
            d.deactivate();
        d.learningState.model.reset();
        d.learningState.modelPath.clear();
        d.learningState.deleteToken.clear();
        d.learningState.status = "模型已删除；采样和其他副本保留";
        d.persist();
        return ok();
    }
    return std::nullopt;
}
} // namespace mousebox::application
