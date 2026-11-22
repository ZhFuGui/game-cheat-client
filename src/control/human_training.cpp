#include "control/human_motion.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
namespace mousebox::control
{
namespace
{
struct Statistics
{
    std::vector<double> speed, acceleration, braking;
    double seconds = 0;
    unsigned trials = 0, excludedWindows = 0, longGaps = 0;
};
double percentile(std::vector<double> values, unsigned p)
{
    if (values.empty())
        return 0;
    std::sort(values.begin(), values.end());
    return values[(values.size() - 1) * p / 100];
}
Statistics extract(const MouseTrial &trial)
{
    Statistics s;
    const auto &points = trial.samples;
    if (trial.session.empty() || trial.session.size() > 200 || trial.dpi < 100 || trial.dpi > 32000 ||
        points.size() < 20 || points.size() > 200000)
        return s;
    if (points.back().us <= points.front().us || points.back().us - points.front().us > 15000000)
        return s;
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        if (!std::isfinite(points[i].x) || !std::isfinite(points[i].y))
            return {};
        if (i && points[i].us < points[i - 1].us)
            return {};
    }
    auto anchor = points.front();
    double previousSpeed = 0;
    bool previousMoving = false;
    for (std::size_t i = 1; i < points.size(); ++i)
    {
        const auto &point = points[i];
        if (point.us - points[i - 1].us > 50000)
        {
            ++s.longGaps;
            anchor = point;
            previousSpeed = 0;
            previousMoving = false;
            continue;
        }
        if (point.us - anchor.us < 20000)
            continue;
        const double dt = static_cast<double>(point.us - anchor.us) / 1000000;
        const double speed = std::hypot(point.x - anchor.x, point.y - anchor.y) * 1600 / trial.dpi / dt;
        if (!std::isfinite(speed) || speed > maximumRecordedSpeed)
        {
            ++s.excludedWindows;
            anchor = point;
            previousSpeed = 0;
            previousMoving = false;
            continue;
        }
        const bool moving = speed >= 40;
        if (moving)
        {
            s.speed.push_back(speed);
            s.seconds += dt;
        }
        if (moving || previousMoving)
        {
            const double delta = (speed - previousSpeed) / dt;
            if (delta > 0 && delta <= 4000000)
                s.acceleration.push_back(delta);
            if (delta < 0 && delta >= -4000000)
                s.braking.push_back(-delta);
        }
        previousSpeed = speed;
        previousMoving = moving;
        anchor = point;
    }
    if (s.speed.size() >= 20)
        s.trials = 1;
    return s;
}
void append(Statistics &a, const Statistics &b)
{
    a.speed.insert(a.speed.end(), b.speed.begin(), b.speed.end());
    a.acceleration.insert(a.acceleration.end(), b.acceleration.begin(), b.acceleration.end());
    a.braking.insert(a.braking.end(), b.braking.begin(), b.braking.end());
    a.seconds += b.seconds;
    a.trials += b.trials;
}
} // namespace
HumanMotionModel trainHumanMotion(const std::vector<MouseTrial> &trials)
{
    if (trials.empty() || trials.size() > 1000)
        throw std::runtime_error("没有有效速度采样");
    std::size_t total = 0;
    std::map<std::string, Statistics> grouped;
    std::vector<std::string> order;
    std::set<std::string> recordedSessions;
    unsigned excludedWindows = 0, longGaps = 0;
    for (const auto &trial : trials)
    {
        total += trial.samples.size();
        if (total > 2000000)
            throw std::runtime_error("采样数量超限");
        recordedSessions.insert(trial.session);
        auto stats = extract(trial);
        excludedWindows += stats.excludedWindows;
        longGaps += stats.longGaps;
        if (!stats.trials)
            continue;
        if (!grouped.contains(trial.session))
            order.push_back(trial.session);
        append(grouped[trial.session], stats);
    }
    // Splitting by start/pause session prevents windows of the same recording from validating each other.
    order.erase(std::remove_if(order.begin(), order.end(), [&](const auto &id) { return grouped[id].seconds < 2; }),
                order.end());
    if (order.size() < 3)
        throw std::runtime_error(
            "已采集 " + std::to_string(recordedSessions.size()) + " 场，其中只有 " + std::to_string(order.size()) +
            " 场达到每场2秒有效移动；需要至少3场。已隔离异常速度窗口 " + std::to_string(excludedWindows) +
            " 个、长采样间隔 " + std::to_string(longGaps) + " 处，请补充未达标场次");
    const auto heldOut = std::max<std::size_t>(1, order.size() / 5);
    Statistics train, test;
    for (std::size_t i = 0; i < order.size(); ++i)
        append(i >= order.size() - heldOut ? test : train, grouped[order[i]]);
    if (train.speed.size() < 200 || test.speed.size() < 100 || train.acceleration.size() < 20 ||
        train.braking.size() < 20)
        throw std::runtime_error("有效速度窗口：训练 " + std::to_string(train.speed.size()) + "/200、验证 " +
                                 std::to_string(test.speed.size()) + "/100；起步 " +
                                 std::to_string(train.acceleration.size()) + "/20、停止 " +
                                 std::to_string(train.braking.size()) + "/20。请补充不足的自然移动或起停样本");
    HumanMotionModel m;
    m.sessions = static_cast<unsigned>(order.size());
    m.trainingSessions = static_cast<unsigned>(order.size() - heldOut);
    m.validationSessions = static_cast<unsigned>(heldOut);
    m.trials = train.trials + test.trials;
    m.trainingRows = static_cast<unsigned>(train.speed.size());
    m.validationRows = static_cast<unsigned>(test.speed.size());
    m.movingSeconds = train.seconds + test.seconds;
    m.speed50 = percentile(train.speed, 50);
    m.speed90 = percentile(train.speed, 90);
    m.speed95 = percentile(train.speed, 95);
    m.acceleration90 = percentile(train.acceleration, 90);
    m.braking90 = percentile(train.braking, 90);
    // Maximum-likelihood log-normal speed fit; this learns a speed distribution, not aiming skill or a path.
    for (auto speed : train.speed)
        m.logSpeedMean += std::log(speed);
    m.logSpeedMean /= train.speed.size();
    for (auto speed : train.speed)
        m.logSpeedSigma += std::pow(std::log(speed) - m.logSpeedMean, 2);
    m.logSpeedSigma = std::sqrt(m.logSpeedSigma / train.speed.size());
    m.validationSpeed90 = percentile(test.speed, 90);
    m.validationRelativeGap = std::abs(m.validationSpeed90 - m.speed90) / m.speed90;
    if (!m.valid())
        throw std::runtime_error("速度档案不合法，请补充正常起停的采样");
    return m;
}
} // namespace mousebox::control
