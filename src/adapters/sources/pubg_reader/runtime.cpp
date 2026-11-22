#include "adapters/sources/pubg_reader/runtime.hpp"
#include "adapters/sources/pubg_reader/session.hpp"
#include "adapters/sources/pubg_reader/transport.hpp"
#include "adapters/sources/pubg_reader/evidence.hpp"
#include "platform/windows/qpc.hpp"
#include "domain/connection_policy.hpp"
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <Windows.h>
#include <bcrypt.h>

namespace mousebox::reader
{
void report(const std::string &text)
{
    static std::mutex mutex;
    const std::lock_guard lock(mutex);
    std::cout << text << std::endl;
}
namespace
{
using Clock = std::chrono::steady_clock;
void delay(unsigned ms, std::stop_token stop)
{
    const auto end = Clock::now() + std::chrono::milliseconds(ms);
    while (!stop.stop_requested() && Clock::now() < end)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
}
std::string summary(const Session &session, const char *category, u64 frequency)
{
    std::ostringstream text;
    const auto current = session.current();
    const auto &timing = session.timing();
    text << category << " status=" << (current ? "CURRENT" : session.ready() ? "WAITING" : "HANDSHAKE");
    if (session.ready())
        text << " source_domain=" << session.binding().clock.domain_id
             << " source_frequency=" << session.binding().clock.qpc_frequency
             << " reader_stream=" << session.binding().session.stream_id
             << " session=" << session.binding().session.session_id;
    if (current)
    {
        text << " epoch=" << current->epoch << " revision=" << current->revision
             << " entities=" << current->entities.size() << " capture=" << timing.capture
             << " source_started=" << timing.started << " source_finished=" << timing.finished;
        if (timing.readFinished)
            text << " source_read_finished=" << *timing.readFinished;
        if (timing.sent)
            text << " source_send_started=" << *timing.sent;
        for (const auto group : {sync::ContextGroup::LocalControl, sync::ContextGroup::ViewPose})
            if (const auto found = current->context.local_groups.find(group);
                found != current->context.local_groups.end())
                text << (group == sync::ContextGroup::LocalControl ? " local_control_status=" : " view_pose_status=")
                     << static_cast<unsigned>(found->second.status);
        text << " local_since_receive_ms="
             << (static_cast<long double>(readQpcTicks() - timing.received) * 1000 / frequency);
    }
    if (const auto &sample = session.probe())
    {
        text << " probe_id=" << sample->id << " net_rtt_ms=" << sample->netRttMs << " t0=" << sample->t0
             << " t1=" << sample->t1 << " t2=" << sample->t2 << " t3=" << sample->t3
             << " probe_age_ms=" << (static_cast<long double>(readQpcTicks() - sample->t3) * 1000 / frequency);
    }
    text << " one_way_ms=unknown invalidations=" << session.invalidationCount();
    text << " baseline_acks=" << session.baselineCount() << " updates=" << session.updateCount()
         << " heartbeat_replies=" << session.heartbeatReplies();
    if (!session.diagnosticError().empty())
        text << " diagnostic=" << session.diagnosticError();
    return text.str();
}
} // namespace
void observe(const Options &options, unsigned categoryValue, std::stop_token stop)
{
    const auto log = [&](const std::string &text) {
        if (options.onLog)
            options.onLog(text);
        else
            report(text);
    };
    const auto publish = [&](const Session *session) {
        if (options.onState)
            options.onState(categoryValue, session);
    };
    const auto connection = [&](ConnectionPhase phase, const std::string &error = {}) {
        if (options.onConnection)
            options.onConnection(categoryValue, phase, error);
    };
    const auto category = static_cast<monitor::DataCategory>(categoryValue);
    const char *name = monitor::CategoryName(category);
    const auto clock = readQpc();
    const std::optional<wire::ClockIdentity> identity =
        options.declareClock
            ? std::optional(wire::ClockIdentity{clock.domain, wire::ClockKind::WindowsQpc, clock.frequency})
            : std::nullopt;
    log(std::string(name) + " LOCAL_CLOCK domain=" + clock.domain + " frequency=" + std::to_string(clock.frequency) +
        " declared=" + (identity ? "yes" : "no") + " consumer_instance=" + options.instance);
    unsigned connections = 0;
    bool reconnectUsed = false;
    while (!stop.stop_requested())
    {
        connection(ConnectionPhase::Connecting);
        try
        {
            Socket socket(options.host, options.port, stop, [&](u64 sent, u64 received) {
                if (options.onTraffic)
                    options.onTraffic(categoryValue, sent, received);
            });
            u64 nonce = 0;
            if (BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(&nonce), sizeof(nonce),
                                BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0 ||
                !nonce)
                throw std::runtime_error("Hello nonce random generation failed");
            Session session(category, identity, options.hz, options.instance, nonce);
            socket.send(session.encode(session.hello()), 1000, stop);
            ++connections;
            log(std::string(name) + " CONNECTED attempt=" + std::to_string(connections));
            Evidence evidence(options.evidenceDirectory, name, connections);
            const auto entered = Clock::now();
            auto lastRecord = entered, nextHeartbeat = entered, nextProbe = entered;
            auto probeStarted = entered, nextPrint = entered;
            auto heartbeatStarted = entered;
            while (!stop.stop_requested())
            {
                const auto now = Clock::now();
                if (!session.ready() && now - entered > std::chrono::seconds(3))
                    throw std::runtime_error("Reader Hello timeout");
                if (session.ready() &&
                    now - lastRecord > std::chrono::milliseconds(session.policy().heartbeat_timeout_ms))
                    throw std::runtime_error("Reader heartbeat timeout");
                if (session.heartbeatPending() &&
                    now - heartbeatStarted > std::chrono::milliseconds(session.policy().heartbeat_timeout_ms))
                    throw std::runtime_error("Reader heartbeat round-trip timeout");
                if (!reconnectUsed && categoryValue == 1 && options.reconnectLivingAfterSeconds && session.ready() &&
                    now - entered >= std::chrono::seconds(options.reconnectLivingAfterSeconds))
                {
                    reconnectUsed = true;
                    log(std::string(name) + " RECONNECT_REQUEST local_socket_close");
                    evidence.snapshot(session);
                    log(summary(session, name, clock.frequency) + " boundary=LOCAL_CLOSE");
                    throw std::runtime_error("Requested Living reconnect");
                }
                if (auto frame = socket.poll(session.maximum(), session.policy().record_progress_timeout_ms, stop))
                {
                    const auto baselineBefore = session.baselineCount(), updateBefore = session.updateCount();
                    const bool readyBefore = session.ready();
                    const auto probeBefore = session.probe();
                    auto replies = session.accept(frame->body, frame->tick);
                    publish(&session); // Revoke an invalidated GUI snapshot before any blocking ACK send.
                    for (auto &reply : replies)
                    {
                        const bool ack = std::holds_alternative<sync::Ack>(reply);
                        auto packet = session.encode(std::move(reply));
                        socket.send(packet, session.policy().record_progress_timeout_ms, stop);
                        if (ack)
                            evidence.sentAck(packet);
                    }
                    if (!readyBefore && session.ready())
                    {
                        evidence.received(frame->body, 6);
                        connection(ConnectionPhase::Connected);
                    }
                    if (baselineBefore != session.baselineCount())
                        evidence.received(frame->body, 19);
                    if (updateBefore != session.updateCount())
                        evidence.received(frame->body, 20);
                    if (session.probe() && (!probeBefore || probeBefore->id != session.probe()->id))
                        evidence.received(frame->body, 18);
                    lastRecord = Clock::now();
                }
                if (session.ready() && now >= nextHeartbeat)
                {
                    if (auto heartbeat = session.prepareHeartbeat(readQpcTicks()))
                    {
                        socket.send(session.encode(*heartbeat), session.policy().record_progress_timeout_ms, stop);
                        heartbeatStarted = Clock::now();
                    }
                    nextHeartbeat = now + std::chrono::milliseconds(session.policy().heartbeat_interval_ms);
                }
                if (session.probePending() &&
                    now - probeStarted > std::chrono::milliseconds(session.policy().heartbeat_timeout_ms))
                {
                    session.expireProbe();
                    publish(&session); // Revoke the GUI prediction clock without waiting for another business record.
                }
                if (options.probes && now >= nextProbe)
                {
                    if (const auto probe = session.prepareProbe())
                    {
                        const auto tick = socket.send(session.encode(*probe),
                                                      session.policy().record_progress_timeout_ms, stop, true);
                        session.probeSent(tick);
                        probeStarted = Clock::now();
                    }
                    nextProbe = now + std::chrono::seconds(1); // At most 1 Hz, one probe in flight.
                }
                if (now >= nextPrint)
                {
                    evidence.snapshot(session);
                    publish(&session);
                    log(summary(session, name, clock.frequency));
                    nextPrint = now + std::chrono::seconds(1);
                }
            }
            evidence.snapshot(session);
            log(summary(session, name, clock.frequency) + " boundary=STOP");
        }
        catch (const std::exception &e)
        {
            publish(nullptr);
            if (stop.stop_requested())
                break;
            connection(ConnectionPhase::Error, e.what());
            log(std::string(name) + " DISCONNECTED current=cleared reason=" + e.what());
            // Session has been destroyed; reconnect cannot retain its baseline, current state or probe.
            delay(AutomaticConnectionIntervalMs, stop);
        }
    }
    publish(nullptr);
    connection(ConnectionPhase::Closed);
    log(std::string(name) + " STOPPED current=cleared");
}
} // namespace mousebox::reader
