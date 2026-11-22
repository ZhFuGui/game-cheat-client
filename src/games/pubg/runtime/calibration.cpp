#include "backend.hpp"
#include "application/ports/output_link.hpp"
#include "platform/windows/input.hpp"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
namespace mousebox::application
{
namespace
{
double difference(double a, double b)
{
    return std::remainder(a - b, 360.0);
}
bool viewFresh(const Snapshot &s, std::uint64_t now)
{
    return s.readerRunning && s.living && s.view && s.view->rotation && s.view->fov &&
           control::checkPair(*s.living, *s.view, {s.sourceSkewMs, 800}).usable() && s.livingObservedMs &&
           s.viewObservedMs && now >= s.viewObservedMs && now >= s.livingObservedMs && now - s.viewObservedMs <= 150 &&
           now - s.livingObservedMs <= 150;
}
} // namespace
void Backend::calibrate(ports::OutputLink &link, std::stop_token stop)
{
    const std::lock_guard cancellation(calibrationMutex_);
    if (!calibrateRequested_ || !sessionEligible_.load())
        return;
    try
    {
        PollTimer timer;
        auto began = monotonicMilliseconds();
        Snapshot initial;
        while (!stop.stop_requested() && calibrateRequested_)
        {
            initial = observeSnapshot();
            if (viewFresh(initial, monotonicMilliseconds()))
                break;
            if (monotonicMilliseconds() - began > 6000)
                throw std::runtime_error("Reader 当前视点尚未就绪");
            timer.wait();
        }
        if (stop.stop_requested() || !calibrateRequested_)
            throw std::runtime_error("校准已取消");
        if (std::abs(initial.view->rotation->x) > 60 || std::abs(initial.view->rotation->z) > 1)
            throw std::runtime_error("请保持视角大致水平后再校准");
        control::Calibration measured{true, initial.measurementProfile, 0, 0, *initial.view->fov};
        std::vector<double> yaw, pitch;
        const auto context = initial.generation;
        for (unsigned trial = 0; trial < 8; ++trial)
        {
            const bool vertical = trial >= 4;
            const int direction = trial % 2 ? -1 : 1;
            const auto before = observeSnapshot();
            if (!viewFresh(before, monotonicMilliseconds()))
                throw std::runtime_error("校准前视点已停滞");
            if (before.generation != context || std::abs(*before.view->fov - measured.fov) > 0.01)
                throw std::runtime_error("校准期间 Reader 上下文发生变化");
            calibrationResult({}, "正在测量 " + std::to_string(trial + 1) + " / 8；请勿移动鼠标", false);
            auto session = randomSession();
            std::uint32_t sequence = 1;
            link.beginPaced(session, 32);
            const auto started = monotonicMilliseconds();
            std::uint64_t lastSend = 0;
            while (monotonicMilliseconds() - started < 400)
            {
                if (stop.stop_requested() || !calibrateRequested_)
                    throw std::runtime_error("校准已取消");
                link.check();
                const auto current = observeSnapshot();
                if (!viewFresh(current, monotonicMilliseconds()) || current.generation != context ||
                    std::abs(*current.view->fov - measured.fov) > 0.01)
                    throw std::runtime_error("校准期间视点失效");
                if (monotonicMilliseconds() - lastSend >= 20)
                {
                    link.refreshPaced(session, ++sequence, static_cast<std::int16_t>(vertical ? 0 : 4 * direction),
                                      static_cast<std::int16_t>(vertical ? 4 * direction : 0), 10, 80);
                    lastSend = monotonicMilliseconds();
                }
                timer.wait();
            }
            link.stop(0);
            link.drain(std::chrono::milliseconds(200));
            const auto settle = monotonicMilliseconds();
            while (monotonicMilliseconds() - settle < 200)
            {
                if (stop.stop_requested() || !calibrateRequested_)
                    throw std::runtime_error("校准已取消");
                timer.wait();
            }
            const auto after = observeSnapshot();
            if (!viewFresh(after, monotonicMilliseconds()) || after.generation != context ||
                after.view->header.sourceFinishNs <= before.view->header.sourceFinishNs ||
                std::abs(*after.view->fov - measured.fov) > 0.01)
                throw std::runtime_error("校准后没有新的有效视点");
            const auto a = *after.view->rotation, b = *before.view->rotation;
            const double delta = vertical ? difference(a.x, b.x) : difference(a.y, b.y);
            const double cross = vertical ? difference(a.y, b.y) : difference(a.x, b.x);
            log("校准 " + std::to_string(trial + 1) + "：" + (vertical ? "pitch " : "yaw ") + std::to_string(delta) +
                "° / " + std::to_string(32 * direction) + " counts；交叉轴 " + std::to_string(cross) + "°");
            if (std::abs(delta) < 0.01 || std::abs(delta) > 15 ||
                std::abs(cross) > std::max(0.05, std::abs(delta) * 0.15))
                throw std::runtime_error("测量位移不足或受到物理鼠标干扰");
            (vertical ? pitch : yaw).push_back(delta / (32 * direction));
        }
        const auto mean = [](const std::vector<double> &samples) {
            const double average = std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
            for (auto n : samples)
                if (n * average <= 0 || std::abs(n - average) > std::abs(average) * 0.15)
                    throw std::runtime_error("往返校准不一致，请重测");
            return average;
        };
        measured.yawDegreesPerCount = mean(yaw);
        measured.pitchDegreesPerCount = mean(pitch);
        if (!control::validCalibration(measured))
            throw std::runtime_error("校准结果不合法");
        calibrationResult(measured, "实机校准完成", true);
        log("实机校准完成；链路就绪后自动恢复侧键跟随。");
    }
    catch (const std::exception &error)
    {
        calibrationResult({}, std::string("校准未完成：") + error.what(), true);
        log(error.what());
    }
    link.stop(0);
    calibrateRequested_ = false;
}
} // namespace mousebox::application
