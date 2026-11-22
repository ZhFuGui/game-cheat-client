#include "platform/windows/framed_socket.hpp"
#include "client.hpp"
#include "adapters/sources/inspection/protobuf_json.hpp"
#include "subscription.hpp"
#include "platform/windows/qpc.hpp"
#include "platform/windows/input.hpp"
#include "domain/connection_policy.hpp"
#include <Windows.h>
#include <atomic>
#include <cmath>
#include <chrono>
#include <deque>
#include <limits>
#include <mutex>
#include <thread>
namespace mousebox::sources::cs2_reader
{
namespace
{
std::atomic<std::uint64_t> nextConnection{1};
void require(bool b, const char *m)
{
    if (!b)
        throw std::runtime_error(m);
}
double ms(std::uint64_t t, std::uint64_t f)
{
    return static_cast<double>(static_cast<long double>(t) * 1000 / f);
}
std::int64_t ns(std::uint64_t t, std::uint64_t f)
{
    auto n = static_cast<long double>(t) * 1000000000 / f;
    require(n <= static_cast<long double>(INT64_MAX), "CS2 clock range exceeded");
    return static_cast<std::int64_t>(n);
}
} // namespace
struct Client::Impl
{
    mutable std::mutex mutex;
    std::jthread worker;
    std::atomic<bool> wanted = false;
    std::atomic<unsigned> maximumAge = 1000;
    std::function<void()> notify;
    Observation state;
    application::ports::SourcePacket inspected;
    Subscription subscription;
    std::optional<control::SourceClockEstimate> mapping;
    std::uint64_t clockMappingRevision = 0;
    std::uint64_t completedEpoch = 0, completedConnection = 0, completedCapture = 0;
    struct Bucket
    {
        std::uint64_t at = 0, up = 0, down = 0;
        unsigned frames = 0, samples = 0;
        double elapsed = 0, uncertainty = 0;
    };
    mutable std::deque<Bucket> buckets;
    domain::ReaderChainMetrics chain;
    void signal()
    {
        std::function<void()> fn;
        {
            std::lock_guard lock(mutex);
            fn = notify;
        }
        if (fn)
            fn();
    }
    Bucket &bucket(std::uint64_t now)
    {
        while (!buckets.empty() && now >= buckets.front().at && now - buckets.front().at >= 1000)
            buckets.pop_front();
        if (buckets.empty() || buckets.back().at != now)
            buckets.push_back({now});
        return buckets.back();
    }
    void traffic(std::uint64_t up, std::uint64_t down)
    {
        std::lock_guard lock(mutex);
        auto &b = bucket(monotonicMilliseconds());
        b.up += up;
        b.down += down;
        if (down)
            subscription.receiveProgress(monotonicMilliseconds());
    }
    void clear(std::string error, bool connecting)
    {
        {
            std::lock_guard lock(mutex);
            state.frame.reset();
            inspected = {};
            chain.reset();
            state.sample.reset();
            state.receipt.reset();
            state.current = false;
            state.connected = false;
            state.verifiedBoneSemantics = false;
            state.boneWorldPose = false;
            state.recoilTimeState = false;
            state.weaponAccuracyEvaluation = false;
            state.connecting = connecting;
            state.error = std::move(error);
            subscription.reset();
            mapping.reset();
            ++clockMappingRevision;
            ++state.revision;
        }
        signal();
    }
    void run(application::ports::SourceConnection options, std::stop_token stop)
    {
        struct Winsock
        {
            WSADATA data{};
            Winsock()
            {
                require(WSAStartup(MAKEWORD(2, 2), &data) == 0, "CS2 Winsock startup failed");
            }
            ~Winsock()
            {
                WSACleanup();
            }
        } winsock;
        while (!stop.stop_requested())
        {
            try
            {
                clear({}, true);
                {
                    std::lock_guard lock(mutex);
                    state.metrics.connection = nextConnection.fetch_add(1);
                }
                platform::Socket socket(options.host, options.port, stop, [this](auto a, auto b) { traffic(a, b); });
                std::optional<Envelope> hello;
                bool subscribed = false, expired = false;
                std::uint64_t event = 0, epoch = 0, capture = 0, serial = 0, query = 0, queryTick = 0;
                const auto localClock = readQpc();
                auto lastEvent = monotonicMilliseconds(), lastQuery = lastEvent;
                const auto sendSubscription = [&] {
                    std::optional<double> rate;
                    std::uint64_t id = 0;
                    {
                        std::lock_guard lock(mutex);
                        if (subscription.needsRequest())
                        {
                            id = ++serial;
                            rate = subscription.begin(id, monotonicMilliseconds());
                        }
                    }
                    if (rate)
                    {
                        socket.send(subscribe(id, *rate), 3000, stop);
                        signal();
                    }
                };
                while (!stop.stop_requested())
                {
                    auto incoming = socket.poll(MaximumRecord, 3000, stop);
                    auto now = monotonicMilliseconds();
                    if (incoming)
                    {
                        auto e = decode(incoming->body);
                        require(e.event > event, "CS2 event sequence regressed");
                        event = e.event;
                        lastEvent = now;
                        if (!hello)
                        {
                            require(e.body == 10, "CS2 first message must be Hello");
                            hello = e;
                            {
                                std::lock_guard lock(mutex);
                                state.verifiedBoneSemantics = e.verifiedBoneSemantics;
                                state.boneWorldPose = e.boneWorldPose;
                                state.recoilTimeState = e.recoilTimeState;
                                state.weaponAccuracyEvaluation = e.weaponAccuracyEvaluation;
                            }
                            sendSubscription();
                            if (e.clockExchange)
                            {
                                query = ++serial;
                                queryTick = socket.send(clockQuery(query), 3000, stop);
                                lastQuery = now;
                            }
                        }
                        else
                        {
                            require(e.instance == hello->instance && e.body != 10,
                                    "CS2 instance changed without reconnect");
                            if (e.body == 12)
                            {
                                {
                                    std::lock_guard lock(mutex);
                                    subscription.confirm(e);
                                    subscribed = true;
                                    state.connected = true;
                                    state.connecting = false;
                                    state.error.clear();
                                    ++state.revision;
                                }
                                signal();
                            }
                            else if (e.body == 13)
                            {
                                require(subscribed, "CS2 snapshot before subscription");
                                const auto f = e.snapshot;
                                require(f && f->instance == hello->instance && f->clockDomain == hello->clockDomain &&
                                            f->frequency == hello->frequency,
                                        "CS2 capture provenance mismatch");
                                validateSnapshotCapabilities(*hello, *f);
                                require(f->epoch >= epoch && (f->epoch != epoch || f->sequence > capture),
                                        "CS2 capture sequence regressed");
                                epoch = f->epoch;
                                capture = f->sequence;
                                expired = false;
                                {
                                    std::lock_guard lock(mutex);
                                    state.frame = f;
                                    state.receivedAtMs = now;
                                    state.error = f->directoryVerified ? "" : "CS2_DIRECTORY_UNVERIFIED";
                                    state.current = f->directoryVerified;
                                    const auto identity = f->instance + ":" + std::to_string(state.metrics.connection) +
                                                          ":" + std::to_string(f->epoch);
                                    inspected = {
                                        "snapshot",
                                        "CS2 完整快照",
                                        identity,
                                        identity + ":" + std::to_string(f->sequence),
                                        "protobuf",
                                        [body = std::make_shared<const std::string>(std::move(incoming->body))] {
                                            return inspection::protobufJson(*body, "cs2.reader.v2.Envelope");
                                        }};
                                    state.sample.reset();
                                    ++bucket(now).frames;
                                    ++state.revision;
                                    state.receipt = domain::SourceReceipt{1,
                                                                          f->instance,
                                                                          epoch,
                                                                          epoch,
                                                                          capture,
                                                                          state.metrics.connection,
                                                                          ms(incoming->tick, localClock.frequency)};
                                    const auto stamp =
                                        [&](std::optional<std::uint64_t> ticks) -> std::optional<std::int64_t> {
                                        if (!ticks)
                                            return {};
                                        const auto value = static_cast<long double>(*ticks) * 1e9L / f->frequency;
                                        if (value < 0 || value >= static_cast<long double>(INT64_MAX))
                                            return {};
                                        return static_cast<std::int64_t>(value);
                                    };
                                    chain.observe({f->instance, f->clockDomain, epoch, capture, stamp(f->started),
                                                   stamp(f->readFinished), stamp(e.sent), mapping,
                                                   ms(incoming->tick, localClock.frequency)});
                                    if (f->started && f->finished && mapping && now >= mapping->calibratedAtMs &&
                                        now - mapping->calibratedAtMs <= 3000)
                                    {
                                        domain::SourceSample sample;
                                        sample.channel = 1;
                                        sample.current = true;
                                        sample.reader = f->instance;
                                        sample.clockDomain = f->clockDomain;
                                        sample.session = epoch;
                                        sample.epoch = epoch;
                                        sample.capture = capture;
                                        sample.consumerConnection = state.metrics.connection;
                                        sample.sourceStartNs = ns(*f->started, f->frequency);
                                        sample.sourceFinishNs = ns(*f->finished, f->frequency);
                                        sample.sourceClock = mapping;
                                        sample.mappingRevision = clockMappingRevision;
                                        state.sample = std::move(sample);
                                    }
                                }
                                signal();
                            }
                            else if (e.body == 14)
                            {
                                if (e.availability != 2)
                                {
                                    {
                                        std::lock_guard lock(mutex);
                                        state.frame.reset();
                                        inspected = {};
                                        chain.reset();
                                        state.sample.reset();
                                        state.receipt.reset();
                                        state.current = false;
                                        state.error = e.error;
                                        ++state.revision;
                                    }
                                    signal();
                                }
                            }
                            else if (e.body == 16)
                            {
                                require(hello->clockExchange && e.request == query && queryTick &&
                                            e.clockDomain == hello->clockDomain && e.frequency == hello->frequency,
                                        "CS2 unexpected clock sample");
                                if (e.received && e.sent)
                                {
                                    require(*e.sent >= *e.received && incoming->tick >= queryTick,
                                            "CS2 clock interval reversed");
                                    const auto localElapsed = ms(incoming->tick - queryTick, localClock.frequency);
                                    const auto remoteElapsed = ms(*e.sent - *e.received, e.frequency);
                                    const auto rtt = localElapsed - remoteElapsed;
                                    if (rtt >= -0.001 && rtt <= 100)
                                    {
                                        control::SourceClockEstimate estimate;
                                        estimate.sourceAnchorNs = ns(*e.received, e.frequency);
                                        estimate.localAnchorMs =
                                            ms(queryTick, localClock.frequency) + std::max(0.0, rtt) / 2;
                                        estimate.uncertaintyMs = std::max(0.0, rtt) / 2 + 0.001;
                                        estimate.calibratedAtMs = now;
                                        {
                                            std::lock_guard lock(mutex);
                                            mapping = estimate;
                                            ++clockMappingRevision;
                                        }
                                    }
                                }
                                queryTick = 0;
                            }
                            else if (e.body == 17)
                            {
                                bool rateRejected;
                                {
                                    std::lock_guard lock(mutex);
                                    rateRejected = subscription.reject(e);
                                    ++state.revision;
                                }
                                if (!rateRejected || !subscribed)
                                    throw std::runtime_error("CS2 Reader rejected: " + e.error);
                                // A rejected rate leaves the confirmed stream and its clock intact.
                                signal();
                            }
                            else
                                require(e.body == 18, "CS2 unsupported server message");
                        }
                    }
                    require(now - lastEvent <= 5000, "CS2 Reader heartbeat timeout");
                    if (hello && hello->clockExchange && now - lastQuery >= 1000)
                    {
                        query = ++serial;
                        queryTick = socket.send(clockQuery(query), 3000, stop);
                        lastQuery = now;
                    }
                    if (hello)
                        sendSubscription();
                    bool changed = false;
                    {
                        std::lock_guard lock(mutex);
                        require(!subscription.expired(now), "CS2 subscription acknowledgement timeout");
                        if (state.frame && !expired && now >= state.receivedAtMs &&
                            now - state.receivedAtMs > maximumAge.load())
                        {
                            expired = true;
                            state.current = false;
                            state.sample.reset();
                            state.error = "CS2_CURRENT_SNAPSHOT_EXPIRED";
                            ++state.revision;
                            changed = true;
                        }
                    }
                    if (changed)
                        signal();
                }
            }
            catch (const std::exception &error)
            {
                if (stop.stop_requested())
                    break;
                clear(error.what(), false);
                const auto end =
                    std::chrono::steady_clock::now() + std::chrono::milliseconds(AutomaticConnectionIntervalMs);
                while (!stop.stop_requested() && std::chrono::steady_clock::now() < end)
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
    }
};
Client::Client() : impl_(std::make_unique<Impl>()) {}
Client::~Client()
{
    disconnect();
}
void Client::connect(const application::ports::SourceConnection &options)
{
    require(!options.host.empty() && options.host.size() <= 255 &&
                options.host.find_first_not_of(" \t\r\n") != std::string::npos && options.port > 0 &&
                std::isfinite(options.hz) && options.hz >= 1 && options.hz <= MaximumSubscriptionHz,
            "Invalid CS2 Reader host, port or subscription frequency");
    disconnect();
    setFrequency(options.hz);
    impl_->wanted = true;
    {
        std::lock_guard lock(impl_->mutex);
        impl_->state.requested = true;
        impl_->state.connecting = true;
    }
    impl_->worker = std::jthread([this, options](std::stop_token stop) {
        try
        {
            impl_->run(options, stop);
        }
        catch (const std::exception &e)
        {
            impl_->clear(e.what(), false);
        }
    });
}
void Client::disconnect()
{
    impl_->wanted = false;
    impl_->worker.request_stop();
    if (impl_->worker.joinable())
        impl_->worker.join();
    {
        std::lock_guard lock(impl_->mutex);
        impl_->state.requested = false;
    }
    impl_->clear({}, false);
}
bool Client::requested() const
{
    return impl_->wanted.load();
}
void Client::setNotifier(std::function<void()> fn)
{
    std::lock_guard lock(impl_->mutex);
    impl_->notify = std::move(fn);
}
void Client::setMaximumAge(unsigned value)
{
    require(value >= 50 && value <= 5000, "CS2 snapshot age range");
    impl_->maximumAge = value;
}
void Client::setFrequency(double hz)
{
    {
        std::lock_guard lock(impl_->mutex);
        impl_->subscription.set(hz);
    }
    impl_->signal();
}
Observation Client::observe() const
{
    std::lock_guard lock(impl_->mutex);
    const auto now = monotonicMilliseconds();
    auto out = impl_->state;
    out.metrics.chain = impl_->chain.snapshot(now);
    out.observedAtMs = now;
    out.requested = requested();
    out.requestedHz = impl_->subscription.requested();
    out.subscribedHz = impl_->subscription.confirmed();
    out.captureHz = impl_->subscription.capture();
    out.subscriptionPending = out.requested && impl_->subscription.waiting();
    out.subscriptionError = impl_->subscription.error();
    out.current =
        out.current && out.frame && now >= out.receivedAtMs && now - out.receivedAtMs <= impl_->maximumAge.load();
    if (!out.current)
        out.sample.reset();
    out.metrics.clock =
        impl_->mapping && now >= impl_->mapping->calibratedAtMs && now - impl_->mapping->calibratedAtMs <= 3000;
    double elapsed = 0, uncertainty = 0;
    for (const auto &b : impl_->buckets)
        if (now >= b.at && now - b.at < 1000)
        {
            out.metrics.upload += static_cast<double>(b.up);
            out.metrics.download += static_cast<double>(b.down);
            out.metrics.frames += b.frames;
            out.metrics.samples += b.samples;
            elapsed += b.elapsed;
            uncertainty += b.uncertainty;
        }
    if (out.metrics.samples)
    {
        out.metrics.averageMs = elapsed / out.metrics.samples;
        out.metrics.uncertaintyMs = uncertainty / out.metrics.samples;
    }
    return out;
}
application::ports::SourcePackets Client::inspectSources() const
{
    const std::lock_guard lock(impl_->mutex);
    if (impl_->state.connected && impl_->state.frame && impl_->inspected.read &&
        monotonicMilliseconds() - impl_->state.receivedAtMs <= impl_->maximumAge.load())
        return {impl_->inspected};
    return {{"snapshot", "CS2 完整快照", "", "", "protobuf", {}}};
}
void Client::completed(std::span<const domain::SourceSample> samples, double completedAtMs)
{
    const auto now = monotonicMilliseconds();
    std::lock_guard lock(impl_->mutex);
    for (const auto &s : samples)
    {
        if (!s.current || !s.sourceClock || !impl_->state.frame || !impl_->state.current ||
            s.reader != impl_->state.frame->instance || s.clockDomain != impl_->state.frame->clockDomain ||
            s.epoch != impl_->state.frame->epoch || s.consumerConnection != impl_->state.metrics.connection)
            continue;
        if (impl_->completedEpoch == s.epoch && impl_->completedConnection == s.consumerConnection &&
            s.capture <= impl_->completedCapture)
            continue;
        const auto &c = *s.sourceClock;
        if (now < c.calibratedAtMs || now - c.calibratedAtMs > 3000)
            continue;
        const auto sourceMs = c.localAnchorMs + static_cast<double>(s.sourceStartNs - c.sourceAnchorNs) / 1000000;
        const auto elapsed = completedAtMs - sourceMs;
        if (!std::isfinite(elapsed) || elapsed < 0 || elapsed > 5000)
            continue;
        impl_->completedEpoch = s.epoch;
        impl_->completedConnection = s.consumerConnection;
        impl_->completedCapture = s.capture;
        auto &b = impl_->bucket(now);
        ++b.samples;
        b.elapsed += elapsed;
        b.uncertainty += c.uncertaintyMs;
    }
}
} // namespace mousebox::sources::cs2_reader
