#include "runtime_service_impl.hpp"
#include "runtime_service_util.hpp"
#include "application/storage/human_store.hpp"
#include "application/assets/asset_paths.hpp"
#include <set>

namespace mousebox::application
{
using namespace runtime_detail;

void RuntimeService::Impl::invalidateDeletion()
{
    ++learningState.modelGeneration;
    learningState.deleteToken.clear();
    learningState.deleteDigest.clear();
    learningState.deletePath.clear();
}

void RuntimeService::Impl::deactivate()
{
    if (learningState.active)
        applyHumanModel(nullptr, values()["dpi"].get<unsigned>(), false);
    learningState.active = false;
}

Json RuntimeService::Impl::learning() const
{
    std::size_t samples = 0;
    std::set<std::string> sessions;
    for (const auto &trial : learningState.trials)
    {
        samples += trial.samples.size();
        sessions.insert(trial.session);
    }
    std::string description = learningState.model
                                  ? "速度中位数 " + number(learningState.model->speed50, 0) + " · 快速上限 " +
                                        number(learningState.model->speed90, 0) + " counts/s（1600 DPI）\n起步 " +
                                        number(learningState.model->acceleration90, 0) + " · 刹车 " +
                                        number(learningState.model->braking90, 0) + " counts/s²\n训练 / 留出场次 " +
                                        std::to_string(learningState.model->trainingSessions) + " / " +
                                        std::to_string(learningState.model->validationSessions) + " · 有效移动 " +
                                        number(learningState.model->movingSeconds, 1) + " 秒 · 留出速度差 " +
                                        number(learningState.model->validationRelativeGap * 100, 1) + "%"
                              : learningState.modelPath.empty() ? "尚无速度模型。载入或拟合后可手动启用。"
                                                                : "已识别旧版点选模型，保持停用，可删除。";
    if (!learningState.modelPath.empty())
        description += "\n" + storage::utf8(learningState.modelPath.wstring());
    const auto state = learningState.training            ? "正在拟合速度和起停幅度…"
                       : learningState.recording         ? "正在采集，先按 Esc 暂停再拟合。"
                       : !learningState.feedback.empty() ? learningState.feedback
                       : sessions.size() < 3
                           ? "尚缺 " + std::to_string(3 - sessions.size()) + " 个独立场次；暂停后再次开始才是新场次。"
                           : "已有 " + std::to_string(sessions.size()) + " 个独立场次，可以拟合并验证。";
    return {{"recording", learningState.recording},
            {"training", learningState.training},
            {"ioBusy", ioBusy},
            {"active", learningState.active},
            {"variation", learningState.variation},
            {"dpi", learningState.captureDpi},
            {"sessions", sessions.size()},
            {"trials", learningState.trials.size()},
            {"samples", samples},
            {"feedback", state},
            {"model", description},
            {"modelPath", storage::utf8(learningState.modelPath.wstring())},
            {"canApply", static_cast<bool>(learningState.model) && !learningState.training && !learningState.recording},
            {"status", learningState.status},
            {"replaying", learningState.replaying},
            {"captureX", learningState.captureX},
            {"captureY", learningState.captureY}};
}

std::optional<Json> RuntimeService::Impl::handleLearningLocked(std::string_view command, const Json &payload)
{
    auto &d = *this;
    if (command == "learning.pause")
        return ok();
    if (command == "learning.clear")
    {
        require(!d.learningState.training, "拟合期间不可清空");
        d.learningState.trials.clear();
        {
            const std::lock_guard input(d.inputMutex);
            d.storedTrialCount = 0;
            d.storedSampleCount = 0;
            d.pendingSampleCount = 0;
            d.pendingTrials.clear();
        }
        d.learningState.lastRecording.clear();
        d.learningState.dirtySamples = false;
        d.learningState.feedback.clear();
        d.learningState.captureX = d.learningState.captureY = 0;
        d.learningState.status = "采样已清空，原始文件保留";
        return ok();
    }
    if (command == "learning.replay")
    {
        require(!d.learningState.training && !d.learningState.trials.empty(), "没有可回放采样");
        d.learningState.replaying = true;
        d.learningState.replayStarted = monotonicMilliseconds();
        d.learningState.status = "回放最近采样（仅视觉显示，不输出鼠标）";
        d.learningState.captureX =
            d.learningState.trials.back().samples.empty() ? 0 : d.learningState.trials.back().samples.back().x;
        d.learningState.captureY =
            d.learningState.trials.back().samples.empty() ? 0 : d.learningState.trials.back().samples.back().y;
        return ok();
    }
    if (command == "learning.setDpi")
    {
        require(!d.learningState.training && !d.learningState.recording, "请先暂停采集或拟合");
        const auto dpi = payload.at("dpi").get<unsigned>();
        require(dpi >= 100 && dpi <= 32000, "采集DPI不合法");
        d.learningState.captureDpi = dpi;
        d.persist();
        return ok();
    }
    if (command == "learning.setVariation")
    {
        require(!d.learningState.training && !d.learningState.recording, "请先暂停采集或拟合");
        const auto variation = payload.at("variation").get<bool>();
        if (d.learningState.variation != variation && d.learningState.active)
            d.applyHumanModel(d.learningState.model, d.values()["dpi"].get<unsigned>(), variation);
        d.learningState.variation = variation;
        d.persist();
        return ok();
    }
    if (command == "samples.import")
    {
        require(!d.learningState.training, "正在拟合");
        std::vector<std::filesystem::path> paths;
        for (const auto &p : payload.at("paths"))
            paths.emplace_back(storage::wide(p.get<std::string>()));
        d.ioBusy = true;
        d.learningState.status = "正在载入采样…";
        d.enqueue([&d, paths = std::move(paths)] {
            try
            {
                auto trials = loadMouseTrialFiles(paths);
                const std::lock_guard done(d.mutex);
                d.learningState.trials = std::move(trials);
                {
                    const std::lock_guard input(d.inputMutex);
                    d.storedTrialCount = d.learningState.trials.size();
                    d.storedSampleCount = 0;
                    for (const auto &trial : d.learningState.trials)
                        d.storedSampleCount += trial.samples.size();
                }
                d.learningState.lastRecording.clear();
                d.learningState.dirtySamples = false;
                d.learningState.feedback.clear();
                d.learningState.status = "采样已载入，原始文件保留";
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
    if (command == "samples.export")
    {
        require(!d.learningState.training && !d.learningState.trials.empty(), "没有可导出的采样");
        const auto path = pathOf(payload);
        const auto trials = d.learningState.trials;
        d.ioBusy = true;
        d.learningState.status = "正在导出采样…";
        d.enqueue([&d, path, trials] {
            try
            {
                require(saveMouseTrials(path, trials), "采样保存失败");
                const std::lock_guard done(d.mutex);
                d.learningState.lastRecording = path;
                d.learningState.status = "采样已导出";
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
    if (command == "learning.fit")
    {
        require(!d.learningState.training, "拟合正在进行");
        std::set<std::string> sessions;
        for (const auto &t : d.learningState.trials)
            sessions.insert(t.session);
        if (sessions.size() < 3)
        {
            d.learningState.feedback = "拟合未开始：当前只有 " + std::to_string(sessions.size()) +
                                       " 个独立场次，需要至少3场。分段不等于独立场次。";
            return ok();
        }
        d.deactivate();
        d.invalidateDeletion();
        if (d.fitWorker.joinable())
            d.fitWorker.join();
        const auto trials = d.learningState.trials;
        d.learningState.training = true;
        d.learningState.fitFinished = false;
        d.learningState.feedback.clear();
        d.fitRunning = true;
        d.fitWorker = std::jthread([&d, trials](std::stop_token stop) {
            struct Completion
            {
                std::atomic<bool> &running;
                ~Completion()
                {
                    running.store(false);
                }
            } completion{d.fitRunning};
            std::shared_ptr<const control::HumanMotionModel> model;
            std::string error;
            try
            {
                model = std::make_shared<const control::HumanMotionModel>(control::trainHumanMotion(trials));
            }
            catch (const std::exception &e)
            {
                error = e.what();
            }
            if (stop.stop_requested())
                return;
            const std::lock_guard complete(d.mutex);
            d.learningState.fitted = std::move(model);
            d.learningState.fitError = std::move(error);
            d.learningState.fitFinished = true;
        });
        return ok();
    }
    return std::nullopt;
}

void RuntimeService::setCaptureHooks(std::function<void(unsigned)> start, std::function<void()> pause,
                                     std::function<void()> clear)
{
    const std::lock_guard lock(impl_->mutex);
    impl_->startCapture = std::move(start);
    impl_->pauseCapture = std::move(pause);
    impl_->clearCapture = std::move(clear);
}

void RuntimeService::acceptCapturedTrials(std::vector<control::MouseTrial> trials, bool recording)
{
    auto &d = *impl_;
    const std::lock_guard lock(d.mutex);
    if (d.closed || d.learningState.training)
        return;
    d.learningState.trials = std::move(trials);
    {
        const std::lock_guard input(d.inputMutex);
        d.storedTrialCount = d.learningState.trials.size();
        d.storedSampleCount = 0;
        for (const auto &trial : d.learningState.trials)
            d.storedSampleCount += trial.samples.size();
    }
    d.learningState.recording = recording;
    d.learningState.dirtySamples = true;
    d.learningState.feedback.clear();
    d.backend.setCapturing(recording);
}

bool RuntimeService::appendCapturedTrials(std::vector<control::MouseTrial> trials)
{
    auto &d = *impl_;
    const std::lock_guard lock(d.inputMutex);
    std::size_t count = 0;
    for (const auto &trial : trials)
        count += trial.samples.size();
    if (d.storedTrialCount + d.pendingTrials.size() + trials.size() > 1000 ||
        d.storedSampleCount + d.pendingSampleCount + count > 2000000)
    {
        d.pendingCapture = Impl::CaptureState{false, "采样已满，请导出并清空", 0, 0};
        return false;
    }
    d.pendingSampleCount += count;
    for (auto &trial : trials)
        d.pendingTrials.push_back(std::move(trial));
    return true;
}

void RuntimeService::setCaptureStatus(bool recording, std::string status, double x, double y)
{
    auto &d = *impl_;
    {
        const std::lock_guard lock(d.inputMutex);
        d.pendingCapture = Impl::CaptureState{recording, std::move(status), x, y};
    }
    d.backend.setCapturing(recording);
}
void RuntimeService::Impl::acceptPendingLearning()
{
    auto &d = *this;
    {
        const std::lock_guard inputLock(d.inputMutex);
        if (d.pendingCapture)
        {
            auto state = std::move(*d.pendingCapture);
            d.pendingCapture.reset();
            d.learningState.recording = state.recording;
            d.learningState.status = std::move(state.status);
            d.learningState.captureX = state.x;
            d.learningState.captureY = state.y;
            if (d.learningState.recording)
                d.learningState.replaying = false;
        }
        if (!d.pendingTrials.empty())
        {
            for (auto &trial : d.pendingTrials)
                d.learningState.trials.push_back(std::move(trial));
            d.pendingTrials.clear();
            d.storedTrialCount = d.learningState.trials.size();
            d.storedSampleCount += d.pendingSampleCount;
            d.pendingSampleCount = 0;
            d.learningState.dirtySamples = true;
            d.learningState.feedback.clear();
        }
    }
}

void RuntimeService::Impl::updateLearningReplay()
{
    auto &d = *this;
    if (d.learningState.replaying && !d.learningState.trials.empty() && !d.learningState.trials.back().samples.empty())
    {
        const auto &points = d.learningState.trials.back().samples;
        const auto until =
            ((monotonicMilliseconds() - d.learningState.replayStarted) * 1000) % (points.back().us + 1000000);
        auto at = std::upper_bound(points.begin(), points.end(), until,
                                   [](std::uint64_t time, const control::MouseSample &p) { return time < p.us; });
        if (at != points.begin())
        {
            --at;
            d.learningState.captureX = at->x;
            d.learningState.captureY = at->y;
        }
    }
}

void RuntimeService::Impl::completeFit()
{
    auto &d = *this;
    if (d.learningState.fitFinished)
    {
        d.learningState.fitFinished = false;
        d.learningState.training = false;
        if (d.learningState.fitted)
        {
            d.invalidateDeletion();
            d.learningState.model = std::move(d.learningState.fitted);
            d.learningState.modelPath = assets::directory(d.options.dataDirectory, assets::Kind::MouseModel) /
                                        storage::wide("speed-model-" + unique() + ".json");
            if (!saveHumanModel(d.learningState.modelPath, *d.learningState.model))
            {
                d.learningState.modelPath.clear();
                d.learningState.feedback = "拟合完成，自动保存失败；请导出模型。";
            }
            else
                d.learningState.feedback = "拟合完成，模型已保存；手动使用才会启用。";
        }
        else
            d.learningState.feedback = "拟合失败：" + d.learningState.fitError + "。已有采样保留。";
        d.persist();
    }
}

void RuntimeService::Impl::saveDirtySamples()
{
    auto &d = *this;
    if (d.learningState.dirtySamples && !d.learningState.savingSamples && !d.learningState.trials.empty())
    {
        d.learningState.dirtySamples = false;
        d.learningState.savingSamples = true;
        if (d.learningState.lastRecording.empty())
            d.learningState.lastRecording = assets::directory(d.options.dataDirectory, assets::Kind::MouseCapture) /
                                            storage::wide("speed-capture-" + unique() + ".json");
        const auto path = d.learningState.lastRecording;
        auto trials = d.learningState.trials;
        d.enqueue([&d, path, trials = std::move(trials)] {
            const bool saved = saveMouseTrials(path, trials);
            const std::lock_guard completed(d.mutex);
            d.learningState.savingSamples = false;
            d.learningState.status =
                saved ? "采样已保存：" + storage::utf8(path.wstring()) : "采样保存失败，请导出到其他目录";
        });
    }
}

} // namespace mousebox::application
