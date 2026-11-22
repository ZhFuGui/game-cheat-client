#pragma once
#include "model/current_facts.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <variant>

namespace monitor::synchronization {
struct SessionKey {
    std::string stream_id, subscriber_id;
    u64 session_id = 0;
    bool operator==(const SessionKey&) const = default;
};
template <class State> struct BasicBaselineOffer {
    SessionKey session;
    u64 generation = 0, fence = 0;
    std::shared_ptr<const State> state;
    // 发送锚点属于传输尝试，不参与不可变基线身份。
    std::optional<u64> send_started_qpc;
};
struct Ack {
    SessionKey session;
    u64 epoch = 0, generation = 0, fence = 0;
    u64 base_revision = 0, applied_revision = 0, capture_sequence = 0;
};
struct Invalidation {
    SessionKey session;
    u64 epoch = 0, fence = 0, revision = 0, capture_sequence = 0;
    bool operator==(const Invalidation&) const = default;
};
template <class Update> using BasicEmission = std::variant<Update, Invalidation>;
struct SenderOptions { std::function<void()> before_commit; };
struct ReceiverOptions {
    std::function<Clock::time_point()> clock = [] { return Clock::now(); };
    std::function<void()> before_commit;
    bool source_clock_is_local = false;
};
enum class ViewStatus { AwaitingBaseline, Current, Rejected };
} // namespace monitor::synchronization
