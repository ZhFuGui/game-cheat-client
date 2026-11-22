#include "capture_payload.hpp"
#include <bit>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace mousebox::desktop::capture
{
namespace
{
void require(bool value)
{
    if (!value)
        throw std::runtime_error("原生采集完成数据不合法");
}
template <typename T> void append(std::vector<std::byte> &bytes, T value)
{
    static_assert(std::endian::native == std::endian::little);
    const auto size = bytes.size();
    bytes.resize(size + sizeof(T));
    std::memcpy(bytes.data() + size, &value, sizeof(T));
}
template <typename T> T take(std::span<const std::byte> &bytes)
{
    require(bytes.size() >= sizeof(T));
    T value;
    std::memcpy(&value, bytes.data(), sizeof(T));
    bytes = bytes.subspan(sizeof(T));
    return value;
}
} // namespace
Frame encodeProgress(const application::MouseReportsProgress &p)
{
    return {{{"event", "reports.progress"},
             {"recording", p.recording},
             {"durationUs", p.durationUs},
             {"reports", p.reports},
             {"other", p.otherDeviceReports},
             {"absolute", p.absoluteReports},
             {"device", p.device},
             {"reason", p.reason}},
            {}};
}
application::MouseReportsProgress decodeProgress(const Json &value)
{
    application::MouseReportsProgress p;
    p.recording = value.at("recording").get<bool>();
    p.durationUs = value.at("durationUs").get<std::uint64_t>();
    p.reports = value.at("reports").get<std::size_t>();
    p.otherDeviceReports = value.at("other").get<std::size_t>();
    p.absoluteReports = value.at("absolute").get<std::size_t>();
    p.device = value.at("device").get<std::string>();
    p.reason = value.at("reason").get<std::string>();
    require(p.reports <= application::MaximumMouseReports && p.device.size() < 32768 && p.reason.size() < 100);
    return p;
}
Frame encodeReports(const application::MouseReportRecording &recording)
{
    require(recording.reports.size() <= application::MaximumMouseReports);
    Frame frame = encodeProgress(recording.progress);
    frame.metadata["event"] = "reports.complete";
    frame.metadata["reports"] = recording.reports.size();
    frame.bytes.reserve(recording.reports.size() * 20);
    for (const auto &p : recording.reports)
    {
        append(frame.bytes, p.us);
        append(frame.bytes, p.dx);
        append(frame.bytes, p.dy);
        append(frame.bytes, p.flags);
        append(frame.bytes, p.buttons);
    }
    return frame;
}
application::MouseReportRecording decodeReports(const Frame &frame)
{
    application::MouseReportRecording value;
    value.progress = decodeProgress(frame.metadata);
    require(!value.progress.recording && frame.bytes.size() == value.progress.reports * 20);
    auto bytes = std::span<const std::byte>(frame.bytes);
    value.reports.reserve(value.progress.reports);
    for (std::size_t i = 0; i < value.progress.reports; ++i)
    {
        application::MouseInputReport report;
        report.us = take<std::uint64_t>(bytes);
        report.dx = take<std::int32_t>(bytes);
        report.dy = take<std::int32_t>(bytes);
        report.flags = take<std::uint16_t>(bytes);
        report.buttons = take<std::uint16_t>(bytes);
        require(report.us <= value.progress.durationUs &&
                (value.reports.empty() || report.us >= value.reports.back().us));
        value.reports.push_back(report);
    }
    return value;
}
Frame encodeTrials(const std::vector<control::MouseTrial> &trials)
{
    Frame frame{{{"event", "learning.trials"}, {"trials", Json::array()}}, {}};
    std::size_t count = 0;
    for (const auto &trial : trials)
        count += trial.samples.size();
    require(trials.size() <= 1000 && count <= application::MaximumMouseReports);
    frame.bytes.reserve(count * 24);
    for (const auto &trial : trials)
    {
        frame.metadata["trials"].push_back(
            {{"dpi", trial.dpi}, {"session", trial.session}, {"count", trial.samples.size()}});
        for (const auto &sample : trial.samples)
        {
            append(frame.bytes, sample.us);
            append(frame.bytes, sample.x);
            append(frame.bytes, sample.y);
        }
    }
    return frame;
}
std::vector<control::MouseTrial> decodeTrials(const Frame &frame)
{
    const auto &list = frame.metadata.at("trials");
    require(list.is_array() && list.size() <= 1000 && frame.bytes.size() <= application::MaximumMouseReports * 24);
    auto bytes = std::span<const std::byte>(frame.bytes);
    std::vector<control::MouseTrial> values;
    for (const auto &metadata : list)
    {
        control::MouseTrial trial;
        trial.dpi = metadata.at("dpi").get<unsigned>();
        trial.session = metadata.at("session").get<std::string>();
        const auto count = metadata.at("count").get<std::size_t>();
        require(trial.dpi >= 100 && trial.dpi <= 32000 && !trial.session.empty() && trial.session.size() <= 200 &&
                count <= 200000 && count <= bytes.size() / 24);
        trial.samples.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            control::MouseSample sample;
            sample.us = take<std::uint64_t>(bytes);
            sample.x = take<double>(bytes);
            sample.y = take<double>(bytes);
            require(sample.us <= 15000000 && (trial.samples.empty() || sample.us >= trial.samples.back().us) &&
                    std::isfinite(sample.x) && std::isfinite(sample.y) && std::abs(sample.x) <= 10000000 &&
                    std::abs(sample.y) <= 10000000);
            trial.samples.push_back(sample);
        }
        values.push_back(std::move(trial));
    }
    require(bytes.empty());
    return values;
}
} // namespace mousebox::desktop::capture
