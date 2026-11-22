#include "source.hpp"
#include "inspection.hpp"
#include "control_projection.hpp"
#include "evidence_loader.hpp"
#include "runtime.hpp"
#include "session.hpp"
#include "games/pubg/module.hpp"
#include "platform/windows/qpc.hpp"

namespace mousebox::reader
{
void observeSource(const SourceOptions &requested, unsigned category, std::stop_token stop)
{
    Options options;
    options.host = requested.host;
    options.port = requested.port;
    options.hz = requested.hz;
    options.instance = requested.instance;
    options.onLog = requested.onLog;
    options.onConnection = requested.onConnection;
    options.onTraffic = requested.onTraffic;
    // This cache and all SDK identities belong to this one source worker.
    std::shared_ptr<const monitor::synchronization::consumer::State> previous;
    std::shared_ptr<const control::Frame> projected;
    const auto localClock = readQpc();
    options.onState = [&](unsigned value, const Session *session) {
        const auto current = session ? session->current() : nullptr;
        const bool changed = current != previous;
        if (!current)
        {
            previous.reset();
            projected.reset();
        }
        else if (changed)
        {
            auto frame = projectControlFrame(*current, session->binding());
            frame.header.sourceClock = session->sourceClockEstimate();
            frame.scene = games::pubg::observeScene(frame);
            projected = std::make_shared<const control::Frame>(std::move(frame));
            previous = current;
        }
        else if (projected && projected->header.sourceClock != session->sourceClockEstimate())
        {
            auto frame = *projected;
            frame.header.sourceClock = session->sourceClockEstimate();
            frame.scene = games::pubg::observeScene(frame);
            projected = std::make_shared<const control::Frame>(std::move(frame));
        }
        std::optional<domain::ReaderChainSample> chain;
        if (session && projected && session->timing().capture == projected->header.capture &&
            session->timing().received)
        {
            const auto &t = session->timing();
            const auto convert = [&](std::optional<std::uint64_t> tick) -> std::optional<std::int64_t> {
                if (!tick)
                    return {};
                monitor::Clock::time_point point;
                monitor::synchronization::codec::Error error;
                if (!monitor::synchronization::codec::FromQpc(session->binding().clock, *tick, point, error))
                    return {};
                return std::chrono::duration_cast<std::chrono::nanoseconds>(point.time_since_epoch()).count();
            };
            chain = domain::ReaderChainSample{
                projected->header.reader,
                projected->header.clockDomain,
                projected->header.epoch,
                t.capture,
                convert(t.started),
                convert(t.readFinished),
                convert(t.sent),
                session->sourceClockEstimate(),
                static_cast<double>(static_cast<long double>(t.received) * 1000 / localClock.frequency)};
        }
        if (requested.onState)
            requested.onState(value,
                              {projected, session && session->ready(), changed,
                               session && session->probe() ? std::optional(session->probe()->netRttMs) : std::nullopt,
                               session ? session->receivedAtMilliseconds() : std::nullopt, std::move(chain),
                               inspectConsumer(session)});
    };
    observe(options, category, stop);
}
void bindSourceLocal(control::Frame &living, const control::Frame *local)
{
    bindLocalControl(living, local);
}
RecordedFrames loadRecordedFrames(const std::filesystem::path &requested)
{
    auto root = requested;
    if (std::filesystem::exists(root / "wire-and-state"))
        root /= "wire-and-state";
    auto living = loadEvidence(root / "Living-2").frame;
    auto local = loadEvidence(root / "Local-1").frame;
    living.scene = games::pubg::observeScene(living);
    local.scene = games::pubg::observeScene(local);
    return {std::move(living), std::move(local)};
}
} // namespace mousebox::reader
