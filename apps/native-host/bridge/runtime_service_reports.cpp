#include "runtime_service_impl.hpp"
#include "runtime_service_util.hpp"
#include "application/assets/asset_paths.hpp"

namespace mousebox::application
{
using namespace runtime_detail;

void RuntimeService::Impl::updateMouseReports()
{
    std::shared_ptr<const MouseReportRecording> ready;
    {
        const std::lock_guard input(inputMutex);
        if (reports.pendingProgress)
        {
            reports.progress = std::move(*reports.pendingProgress);
            reports.pendingProgress.reset();
        }
        ready = std::move(reports.pendingData);
    }
    if (!ready)
        return;
    reports.progress = ready->progress;
    reports.data = ready;
    reports.analyzing = true;
    reports.error.clear();
    const auto id = std::to_string(reports.revision);
    const auto path = assets::directory(options.dataDirectory, assets::Kind::MouseReports) /
                      storage::wide("reports-" + unique() + ".csv");
    try
    {
        enqueue([this, ready, id, path] {
            Json result;
            std::string error;
            try
            {
                result = analyzeMouseReports(*ready);
                result["reportId"] = id;
                const bool saved = saveMouseReportsCsv(path, *ready);
                result["savedPath"] = saved ? Json(storage::utf8(path.wstring())) : Json{};
                if (!saved || !storage::save(std::filesystem::path(path.wstring() + L".json"), result))
                    error = "鼠标特征自动保存失败，请导出原始报文。";
            }
            catch (const std::exception &e)
            {
                error = e.what();
            }
            const std::lock_guard complete(mutex);
            reports.result = std::move(result);
            reports.analyzing = false;
            reports.error = std::move(error);
            log("鼠标特征统计完成 · " + std::to_string(ready->reports.size()) + " 条原始报文");
        });
    }
    catch (const std::exception &e)
    {
        reports.analyzing = false;
        reports.error = e.what();
    }
}

Json RuntimeService::Impl::reportState() const
{
    auto value = mouseReportsProgressJson(reports.progress);
    value["analyzing"] = reports.analyzing;
    value["busy"] = ioBusy;
    value["revision"] = std::to_string(reports.revision);
    value["hasResult"] = !reports.result.is_null();
    value["canExport"] =
        reports.data && !reports.data->reports.empty() && !reports.progress.recording && !reports.analyzing;
    value["error"] = reports.error;
    return value;
}

std::optional<Json> RuntimeService::Impl::handleReportsLocked(std::string_view command, const Json &payload)
{
    auto &d = *this;
    if (command == "mouseReports.clear")
    {
        require(!d.reports.progress.recording && !d.reports.analyzing && !d.ioBusy, "请先结束当前采集或后台任务");
        d.reports.data.reset();
        d.reports.result = nullptr;
        d.reports.progress = {};
        d.reports.error.clear();
        ++d.reports.revision;
        return ok();
    }
    if (command == "mouseReports.export")
    {
        require(!d.reports.progress.recording && !d.reports.analyzing && !d.ioBusy && d.reports.data &&
                    !d.reports.data->reports.empty(),
                "没有可导出的鼠标报文");
        const auto path = pathOf(payload);
        const auto data = d.reports.data;
        d.ioBusy = true;
        d.reports.error.clear();
        d.enqueue([&d, data, path] {
            const bool saved = saveMouseReportsCsv(path, *data);
            const std::lock_guard complete(d.mutex);
            d.ioBusy = false;
            if (!saved)
                d.reports.error = "鼠标报文导出失败，请选择其他位置。";
        });
        return ok();
    }
    return std::nullopt;
}

void RuntimeService::setReportCaptureHook(std::function<void()> start)
{
    const std::lock_guard lock(impl_->mutex);
    impl_->startReportCapture = std::move(start);
}

void RuntimeService::setReportCaptureStatus(MouseReportsProgress progress)
{
    auto &d = *impl_;
    const bool active = progress.recording;
    {
        const std::lock_guard lock(d.inputMutex);
        d.reports.pendingProgress = std::move(progress);
    }
    d.backend.setCapturing(active);
}

void RuntimeService::acceptMouseReports(MouseReportRecording recording)
{
    impl_->backend.setCapturing(false);
    auto data = std::make_shared<const MouseReportRecording>(std::move(recording));
    const std::lock_guard lock(impl_->inputMutex);
    impl_->reports.pendingProgress = data->progress;
    impl_->reports.pendingData = std::move(data);
}
} // namespace mousebox::application
