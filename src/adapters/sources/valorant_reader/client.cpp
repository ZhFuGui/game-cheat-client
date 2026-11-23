#include "platform/windows/framed_socket.hpp"
#include "client.hpp"
#include "adapters/sources/inspection/protobuf_json.hpp"
#include "platform/windows/qpc.hpp"
#include "platform/windows/input.hpp"
#include "domain/connection_policy.hpp"
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <set>
#include <thread>

namespace mousebox::sources::valorant_reader
{
using namespace wire;
namespace
{
std::atomic<std::uint64_t> nextConnection{1};
double milliseconds(std::uint64_t ticks, std::uint64_t frequency)
{
    return static_cast<double>(static_cast<long double>(ticks) * 1000 / frequency);
}
std::optional<std::int64_t> nanoseconds(std::optional<std::uint64_t> ticks, std::uint64_t frequency)
{
    if (!ticks)
        return {};
    const auto value = static_cast<long double>(*ticks) * 1000000000 / frequency;
    return value <= INT64_MAX ? std::optional(static_cast<std::int64_t>(value)) : std::nullopt;
}
std::string reason(std::uint64_t value)
{
    constexpr const char *descriptions[]{"请求格式或身份无效",   "协议版本不匹配",         "协议摘要不匹配",
                                         "订阅类别或投影未实现", "请求频率超出范围",       "配置版本已变化",
                                         "时钟映射候选无效",     "资源预算超限或编码失败", "采集会话暂不可用"};
    if (!value)
        return "VALORANT Reader 等待可用采集会话";
    return "VALORANT Reader R" + std::to_string(value) +
           (value >= 1001 && value <= 1009 ? std::string(" · ") + descriptions[value - 1001] : "");
}
void pause(std::stop_token stop, unsigned duration)
{
    const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration);
    while (!stop.stop_requested() && std::chrono::steady_clock::now() < until)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
}
} // namespace
struct Client::Impl
{
    mutable std::mutex mutex;
    std::array<std::jthread, 3> workers;
    std::atomic<bool> wanted = false;
    std::atomic<std::uint64_t> generation = 1;
    Observation state;
    std::function<void()> notify;
    std::array<application::ports::SourcePacket, 3> packets;
    std::array<domain::ReaderChainMetrics, 3> chains;
    struct Traffic
    {
        std::uint64_t at, up = 0, down = 0;
        std::array<unsigned, 3> frames{};
    };
    mutable std::deque<Traffic> traffic;
    std::string instance, clockDomain;
    std::uint64_t clockFrequency = 0, clockRevision = 0;
    std::set<std::string> retiredInstances;
    std::uint64_t epoch = 0;
    bool unavailable = false;
    void signal()
    {
        std::function<void()> fn;
        {
            const std::lock_guard lock(mutex);
            fn = notify;
        }
        if (fn)
            fn();
    }
    Traffic &bucket(std::uint64_t now)
    {
        while (!traffic.empty() && now >= traffic.front().at && now - traffic.front().at >= 1000)
            traffic.pop_front();
        if (traffic.empty() || traffic.back().at != now)
            traffic.push_back({now});
        return traffic.back();
    }
    void clear(unsigned index)
    {
        state.channels[index].data.reset();
        constexpr const char *names[]{"VALORANT · 人物位置", "VALORANT · 人物骨骼", "VALORANT · 相机"};
        packets[index] = {"valorant-" + std::to_string(index + 1), names[index], {}, {}, "protobuf", {}};
        chains[index].reset();
        ++state.revision;
    }
    void run(unsigned index, application::ports::SourceConnection options, std::stop_token stop)
    {
        struct Winsock
        {
            WSADATA data{};
            Winsock()
            {
                require(WSAStartup(MAKEWORD(2, 2), &data) == 0, "VALORANT Winsock initialization failed");
            }
            ~Winsock()
            {
                WSACleanup();
            }
        };
        const unsigned stream = index + 1;
        while (!stop.stop_requested())
        {
            try
            {
                Winsock winsock;
                const auto local = readQpc();
                const auto connection = nextConnection.fetch_add(1);
                {
                    const std::lock_guard lock(mutex);
                    clear(index);
                    auto &channel = state.channels[index];
                    channel.connection = connection;
                    channel.connected = false;
                    channel.connecting = true;
                    channel.error.clear();
                    channel.subscriptionError.clear();
                    channel.subscribedHz.reset();
                    channel.pending = true;
                }
                signal();
                auto joined = generation.load();
                platform::Socket socket(options.host, options.port, stop, [this](auto up, auto down) {
                    const std::lock_guard lock(mutex);
                    auto &value = bucket(monotonicMilliseconds());
                    value.up += up;
                    value.down += down;
                });
                std::optional<Hello> hello;
                StreamState baseline;
                std::uint64_t event = 0, serial = 0, requestId = 0, revision = 0;
                std::uint64_t queryId = 0, queryTick = 0, candidateVersion = 0, selectId = 0, selectedVersion = 0;
                std::optional<control::SourceClockEstimate> candidate, mapping;
                double sentRate = 0;
                bool configured = false, resyncPending = false;
                auto lastEvent = monotonicMilliseconds(), lastClock = lastEvent;
                auto controlStarted = lastEvent;
                while (!stop.stop_requested())
                {
                    require(joined == generation.load(), "VALORANT source session changed");
                    const auto now = monotonicMilliseconds();
                    require(now - lastEvent < 10000, "VALORANT Reader connection timed out");
                    require(!requestId || now - controlStarted < 5000, "VALORANT Reader subscription timed out");
                    double desired;
                    {
                        const std::lock_guard lock(mutex);
                        desired = state.channels[index].requestedHz;
                    }
                    if (hello && configured && !requestId && desired != sentRate)
                    {
                        requestId = ++serial;
                        sentRate = desired;
                        controlStarted = now;
                        socket.send(rate(requestId, revision, desired), 2000, stop);
                        const std::lock_guard lock(mutex);
                        state.channels[index].pending = true;
                        state.channels[index].subscriptionError.clear();
                    }
                    if (hello && configured && hello->clockExchange && !queryId && !selectId && now - lastClock >= 1000)
                    {
                        ++candidateVersion;
                        queryId = ++serial;
                        std::string value;
                        number(value, 1, queryId);
                        number(value, 2, candidateVersion);
                        queryTick = socket.send(request(18, value), 2000, stop);
                        lastClock = now;
                    }
                    const auto incoming = socket.poll(MaximumRecord, 5000, stop);
                    if (!incoming)
                        continue;
                    const auto receivedMs = milliseconds(incoming->tick, local.frequency);
                    const auto envelope = decode(incoming->body);
                    require(envelope.event > event, "VALORANT Reader event sequence did not increase");
                    event = envelope.event;
                    lastEvent = monotonicMilliseconds();
                    const auto &fields = envelope.payload;
                    if (!hello)
                    {
                        hello = decodeHello(envelope);
                        require(hello->streams[stream] && desired <= hello->maximumHz,
                                "VALORANT stream/rate unsupported");
                        {
                            const std::lock_guard lock(mutex);
                            require(!retiredInstances.contains(hello->instance), "VALORANT retired Reader instance");
                            if (!instance.empty() && instance != hello->instance)
                            {
                                retiredInstances.insert(instance);
                                for (unsigned i = 0; i < 3; ++i)
                                    clear(i);
                                epoch = 0;
                                clockDomain.clear();
                                ++generation;
                            }
                            instance = hello->instance;
                            if (clockDomain.empty())
                            {
                                clockDomain = hello->domain;
                                clockFrequency = hello->frequency;
                                clockRevision = hello->clockRevision;
                            }
                            require(clockDomain == hello->domain && clockFrequency == hello->frequency &&
                                        clockRevision == hello->clockRevision,
                                    "VALORANT cross-stream clock identity mismatch");
                            joined = generation.load();
                            state.channels[index].instance = instance;
                        }
                        requestId = ++serial;
                        sentRate = desired;
                        controlStarted = lastEvent;
                        socket.send(subscribe(requestId, stream, desired), 2000, stop);
                    }
                    else if (envelope.body == 13)
                    {
                        require(requestId && integer(fields, 1) == requestId && integer(fields, 2) == stream &&
                                    integer(fields, 3) == 1,
                                "VALORANT invalid Configured receipt");
                        const auto accepted = optionalReal(fields, 4, 1).value_or(0);
                        require(accepted == sentRate && integer(fields, 5) > 0 && integer(fields, 7) > 0,
                                "VALORANT invalid adopted rate/version");
                        revision = integer(fields, 7);
                        configured = true;
                        requestId = 0;
                        const std::lock_guard lock(mutex);
                        auto &channel = state.channels[index];
                        channel.connected = true;
                        channel.connecting = false;
                        channel.subscribedHz = accepted;
                        channel.pending = channel.requestedHz != accepted;
                        channel.subscriptionError.clear();
                        ++state.revision;
                    }
                    else if (envelope.body == 14)
                    {
                        require(configured, "VALORANT data arrived before subscription confirmation");
                        if (!applyUpdate(envelope, stream, hello->clockRevision, baseline))
                        {
                            baseline.baseline = false;
                            {
                                const std::lock_guard lock(mutex);
                                clear(index);
                            }
                            if (!resyncPending)
                            {
                                std::string value;
                                number(value, 1, ++serial);
                                socket.send(request(22, value), 2000, stop);
                                resyncPending = true;
                            }
                            continue;
                        }
                        resyncPending = false;
                        const auto accepted = std::make_shared<const StreamState>(baseline);
                        const auto raw = std::make_shared<const std::string>(incoming->body);
                        const std::lock_guard lock(mutex);
                        require(joined == generation.load() && instance == hello->instance,
                                "VALORANT late source session data");
                        if (accepted->capture.epoch < epoch)
                            continue;
                        if (accepted->capture.epoch > epoch)
                        {
                            epoch = accepted->capture.epoch;
                            for (unsigned i = 0; i < 3; ++i)
                                clear(i);
                        }
                        unavailable = false;
                        auto &channel = state.channels[index];
                        channel.data = accepted;
                        channel.receivedAtMs = lastEvent;
                        channel.error.clear();
                        bucket(lastEvent).frames[index]++;
                        chains[index].observe(
                            {hello->instance, hello->domain, epoch, accepted->capture.sequence,
                             nanoseconds(accepted->capture.started, hello->frequency),
                             nanoseconds(accepted->capture.readFinished, hello->frequency),
                             nanoseconds(envelope.sent, hello->frequency),
                             envelope.mapping == selectedVersion && selectedVersion ? mapping : std::nullopt,
                             receivedMs});
                        packets[index] = {
                            "valorant-" + std::to_string(stream),
                            "VALORANT " + std::to_string(stream),
                            hello->instance + ":" + std::to_string(epoch) + ":" + std::to_string(connection),
                            std::to_string(accepted->capture.sequence),
                            "protobuf",
                            [raw] { return inspection::protobufJson(*raw, "valorant.reader.v1.Envelope"); }};
                        ++state.revision;
                    }
                    else if (envelope.body == 16)
                    {
                        const auto availability = integer(fields, 1);
                        const auto statusEpoch = optionalInteger(fields, 2);
                        const bool global = integer(fields, 3) != 0;
                        const std::lock_guard lock(mutex);
                        if (statusEpoch && *statusEpoch < epoch)
                            continue;
                        if (statusEpoch && *statusEpoch > epoch)
                        {
                            epoch = *statusEpoch;
                            for (unsigned i = 0; i < 3; ++i)
                                clear(i);
                        }
                        if (availability != 2)
                        {
                            baseline = {};
                            clear(index);
                            state.channels[index].error = reason(integer(fields, 4));
                            if (global && !unavailable)
                            {
                                unavailable = true;
                                for (unsigned i = 0; i < 3; ++i)
                                    clear(i);
                                ++generation;
                            }
                        }
                    }
                    else if (envelope.body == 17)
                    {
                        const auto rejected = integer(fields, 1);
                        require(rejected && integer(fields, 2), "VALORANT malformed rejection");
                        if (rejected == requestId && configured)
                        {
                            const std::lock_guard lock(mutex);
                            state.channels[index].subscriptionError = reason(integer(fields, 2));
                            state.channels[index].pending = false;
                            requestId = 0;
                            // Preserve the last confirmed subscription. A new edit
                            // can retry; the rejected desired rate is not retried in a loop.
                        }
                        else if (rejected == selectId || rejected == queryId)
                        {
                            selectId = queryId = 0;
                            candidate.reset();
                            lastClock = lastEvent;
                        }
                        else
                            throw std::runtime_error(reason(integer(fields, 2)));
                    }
                    else if (envelope.body == 19)
                    {
                        require(integer(fields, 1) == queryId && queryId && integer(fields, 2) == candidateVersion &&
                                    integer(fields, 3) == hello->clockRevision,
                                "VALORANT unexpected clock sample");
                        const auto t1 = optionalInteger(fields, 4, 1);
                        queryId = 0;
                        if (t1 && envelope.sent && *envelope.sent >= *t1 && incoming->tick >= queryTick)
                        {
                            const auto rtt = milliseconds(incoming->tick - queryTick, local.frequency) -
                                             milliseconds(*envelope.sent - *t1, hello->frequency);
                            const auto anchor = nanoseconds(t1, hello->frequency);
                            if (anchor && rtt >= -0.001 && rtt <= 1000)
                            {
                                candidate = control::SourceClockEstimate{
                                    *anchor, milliseconds(queryTick, local.frequency) + std::max(0.0, rtt) / 2,
                                    std::max(0.0, rtt) / 2 + 0.001, lastEvent};
                                selectId = ++serial;
                                std::string value;
                                number(value, 1, selectId);
                                number(value, 2, candidateVersion);
                                number(value, 3, hello->clockRevision);
                                socket.send(request(20, value), 2000, stop);
                            }
                        }
                    }
                    else if (envelope.body == 21)
                    {
                        require(selectId && integer(fields, 1) == selectId && candidate &&
                                    integer(fields, 2) == candidateVersion,
                                "VALORANT invalid clock selection receipt");
                        selectedVersion = candidateVersion;
                        mapping = candidate;
                        selectId = 0;
                    }
                    else
                    {
                        require(envelope.body == 23, "VALORANT unexpected repeated Hello");
                        if (auto knownEpoch = optionalInteger(fields, 3))
                        {
                            const std::lock_guard lock(mutex);
                            if (*knownEpoch > epoch)
                            {
                                epoch = *knownEpoch;
                                for (unsigned i = 0; i < 3; ++i)
                                    clear(i);
                            }
                        }
                    }
                    signal();
                }
            }
            catch (const std::exception &error)
            {
                {
                    const std::lock_guard lock(mutex);
                    clear(index);
                    auto &channel = state.channels[index];
                    channel.connected = false;
                    channel.connecting = false;
                    channel.pending = false;
                    channel.error = error.what();
                }
                signal();
                pause(stop, AutomaticConnectionIntervalMs);
            }
        }
    }
};
Client::Client() : impl_(std::make_unique<Impl>())
{
    for (unsigned i = 0; i < 3; ++i)
        impl_->clear(i);
}
Client::~Client()
{
    setNotifier({});
    disconnect();
}
void Client::connect(const application::ports::SourceConnection &options)
{
    require(!options.host.empty() && options.host.size() <= 255 && options.port && std::isfinite(options.hz) &&
                options.hz >= 1 && options.hz <= 300,
            "VALORANT connection parameters invalid");
    disconnect();
    impl_->wanted = true;
    {
        const std::lock_guard lock(impl_->mutex);
        impl_->state.channels[0].requestedHz = options.hz;
        impl_->instance.clear();
        impl_->clockDomain.clear();
        impl_->retiredInstances.clear();
        impl_->epoch = 0;
        impl_->unavailable = false;
    }
    for (unsigned i = 0; i < 3; ++i)
        impl_->workers[i] = std::jthread([this, i, options](std::stop_token stop) { impl_->run(i, options, stop); });
}
void Client::disconnect()
{
    impl_->wanted = false;
    for (auto &worker : impl_->workers)
        worker.request_stop();
    for (auto &worker : impl_->workers)
        if (worker.joinable())
            worker.join();
    {
        const std::lock_guard lock(impl_->mutex);
        for (unsigned i = 0; i < 3; ++i)
        {
            impl_->clear(i);
            const auto hz = impl_->state.channels[i].requestedHz;
            impl_->state.channels[i] = {};
            impl_->state.channels[i].requestedHz = hz;
        }
        impl_->traffic.clear();
    }
    impl_->signal();
}
bool Client::requested() const
{
    return impl_->wanted;
}
void Client::setNotifier(std::function<void()> fn)
{
    const std::lock_guard lock(impl_->mutex);
    impl_->notify = std::move(fn);
}
void Client::setRates(std::array<double, 3> values)
{
    for (auto value : values)
        require(std::isfinite(value) && value >= 1 && value <= 300, "VALORANT rate out of range");
    const std::lock_guard lock(impl_->mutex);
    for (unsigned i = 0; i < 3; ++i)
        impl_->state.channels[i].requestedHz = values[i];
}
Observation Client::observe() const
{
    const std::lock_guard lock(impl_->mutex);
    auto out = impl_->state;
    out.requested = impl_->wanted;
    const auto now = monotonicMilliseconds();
    for (unsigned i = 0; i < 3; ++i)
    {
        out.channels[i].chain = impl_->chains[i].snapshot(now);
        out.channels[i].framesPerSecond = 0;
    }
    out.upload = out.download = 0;
    for (const auto &value : impl_->traffic)
        if (now >= value.at && now - value.at < 1000)
        {
            out.upload += static_cast<double>(value.up);
            out.download += static_cast<double>(value.down);
            for (unsigned i = 0; i < 3; ++i)
                out.channels[i].framesPerSecond += value.frames[i];
        }
    return out;
}
application::ports::SourcePackets Client::inspectSources() const
{
    const std::lock_guard lock(impl_->mutex);
    application::ports::SourcePackets out;
    for (const auto &packet : impl_->packets)
        out.push_back(packet);
    return out;
}
} // namespace mousebox::sources::valorant_reader
