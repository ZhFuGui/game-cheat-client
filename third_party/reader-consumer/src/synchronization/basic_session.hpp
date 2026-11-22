#pragma once
#include "synchronization/session_types.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace monitor::synchronization {
// 两种投影仅替换模型策略；ACK、栅栏、失败下界与提交复验使用同一实现。
namespace session_detail {

inline void ValidSession(const SessionKey& session) {
    if (session.stream_id.empty() || session.subscriber_id.empty() || !session.session_id)
        throw std::invalid_argument("Synchronization requires a unique stream/subscriber/session");
}
inline u64 Advance(u64 value) {
    if (value == (std::numeric_limits<u64>::max)())
        throw std::overflow_error("Synchronization generation exhausted");
    return value + 1;
}
inline bool Regressed(u64 epoch, u64 revision, u64 capture, u64 active_epoch, u64 highest_revision, u64 highest_capture) {
    return epoch < active_epoch ||
           (epoch == active_epoch && (revision < highest_revision || (capture && capture < highest_capture)));
}

} // namespace session_detail
template <class Policy> class BasicSenderSession {
  public:
    using State = typename Policy::State;
    using Update = typename Policy::Update;
    using FrameContext = typename Policy::Context;
    using BaselineOffer = BasicBaselineOffer<State>;
    using Emission = BasicEmission<Update>;

    explicit BasicSenderSession(SessionKey session, SenderOptions options = {})
        : session_(std::move(session)), options_(std::move(options)) {
        session_detail::ValidSession(session_);
    }
    BaselineOffer OfferBaseline(std::shared_ptr<const State> state) {
        if (!state)
            throw std::invalid_argument("Missing baseline");
        auto owned = std::make_shared<const State>(*state);
        std::string error;
        if (!owned->accepted || !Policy::Validate(*owned, error))
            throw std::invalid_argument(error.empty() ? "Invalid baseline" : error);
        BaselineOffer offer{session_, 0, 0, owned};
        if (options_.before_commit)
            options_.before_commit();
        std::lock_guard lock(mutex_);
        if (session_detail::Regressed(owned->epoch, owned->revision, owned->capture_sequence, epoch_, highest_revision_,
                      highest_capture_) ||
            (owned->epoch == epoch_ && (owned->revision <= invalidated_revision_ ||
                                        (failed_capture_floor_ && owned->capture_sequence <= failed_capture_floor_))))
            throw std::invalid_argument("Baseline does not cross the current failure/revision barrier");
        // 相同版本重新提供必须保持原来的不可变内容，不能把一次采集改写成另一个状态。
        const auto known = pending_ && pending_->revision == owned->revision ? pending_ : acknowledged_;
        if (owned->epoch == epoch_ && owned->revision == highest_revision_ &&
            (!known || known->epoch != owned->epoch || known->revision != owned->revision || !Policy::Same(*owned, *known)))
            throw std::invalid_argument("Baseline revision has no proven identical immutable state");
        if (owned->epoch == epoch_ && owned->revision > highest_revision_ && owned->capture_sequence <= highest_capture_)
            throw std::invalid_argument("New baseline revision requires a new capture sequence");
        FrameContext staged_identity;
        if (!Policy::MergeIdentity(owned->epoch == epoch_ ? identity_fence_ : FrameContext{}, owned->context, staged_identity,
                                error))
            throw std::invalid_argument(error);
        offer.generation = session_detail::Advance(generation_);
        offer.fence = session_detail::Advance(fence_);
        if (owned->epoch != epoch_) {
            invalidated_revision_ = 0;
            failed_capture_floor_ = 0;
        }
        generation_ = offer.generation;
        fence_ = offer.fence;
        epoch_ = owned->epoch;
        pending_ = std::move(owned);
        waiting_ack_ = true;
        highest_revision_ = pending_->revision;
        highest_capture_ = pending_->capture_sequence;
        identity_fence_.swap(staged_identity);
        return offer;
    }
    bool AcceptAck(const Ack& ack) {
        std::lock_guard lock(mutex_);
        if (ack.session != session_ || !pending_ || ack.epoch != epoch_ || ack.generation != generation_ ||
            ack.fence != fence_ || ack.base_revision != pending_->revision || ack.applied_revision != pending_->revision ||
            ack.capture_sequence != pending_->capture_sequence)
            return false;
        if (!waiting_ack_)
            return acknowledged_ == pending_;
        acknowledged_ = pending_;
        waiting_ack_ = false;
        // 新基线原子安装的精确 ACK 同时证明旧失效栅栏已被跨越。
        invalidation_.reset();
        return true;
    }
    bool Ready() const {
        std::lock_guard lock(mutex_);
        return !waiting_ack_ && bool(acknowledged_);
    }
    std::optional<Emission> MakeUpdate(const State& current) {
        if (!current.accepted) {
            std::lock_guard lock(mutex_);
            if (auto event = InvalidateLocked(current.epoch, current.revision, current.capture_sequence))
                return Emission{std::move(*event)};
            return std::nullopt;
        }
        std::shared_ptr<const State> baseline;
        u64 epoch = 0, generation = 0, fence = 0;
        {
            std::lock_guard lock(mutex_);
            if (waiting_ack_ || !acknowledged_ || current.epoch != epoch_ || current.revision <= highest_revision_ ||
                current.capture_sequence <= highest_capture_)
                return std::nullopt;
            baseline = acknowledged_;
            epoch = epoch_;
            generation = generation_;
            fence = fence_;
        }
        // 失效可抢占耗时的差分构建；最后必须证明仍是同一已确认基线。
        Emission result{Policy::BuildAcknowledgedUpdate(*baseline, current, generation, fence)};
        if (options_.before_commit)
            options_.before_commit();
        std::lock_guard lock(mutex_);
        if (waiting_ack_ || acknowledged_ != baseline || epoch_ != epoch || generation_ != generation || fence_ != fence ||
            current.revision <= highest_revision_ || current.capture_sequence <= highest_capture_)
            return std::nullopt;
        FrameContext staged_identity;
        std::string error;
        if (!Policy::MergeIdentity(identity_fence_, current.context, staged_identity, error))
            throw std::invalid_argument(error);
        highest_revision_ = current.revision;
        highest_capture_ = current.capture_sequence;
        identity_fence_.swap(staged_identity);
        return result;
    }
    std::optional<Invalidation> Invalidate(u64 epoch, u64 revision, u64 capture) {
        std::lock_guard lock(mutex_);
        return InvalidateLocked(epoch, revision, capture);
    }
    std::optional<Invalidation> PendingInvalidation() const {
        std::lock_guard lock(mutex_);
        return invalidation_;
    }

  private:
    std::optional<Invalidation> InvalidateLocked(u64 epoch, u64 revision, u64 capture) {
        if (!epoch || !revision)
            throw std::invalid_argument("Invalidation requires epoch/revision");
        if (invalidation_ && invalidation_->epoch == epoch && invalidation_->revision == revision &&
            invalidation_->capture_sequence == capture)
            return invalidation_;
        if (session_detail::Regressed(epoch, revision, capture, epoch_, highest_revision_, highest_capture_) ||
            (epoch == epoch_ && revision <= highest_revision_))
            return std::nullopt;
        Invalidation event{session_, epoch, session_detail::Advance(fence_), revision, capture};
        auto retained = event; // 所有可能抛出的分配先完成，之后只提交标量和 noexcept move。
        const auto same_epoch = epoch == epoch_;
        invalidation_ = std::move(retained);
        fence_ = event.fence;
        epoch_ = epoch;
        invalidated_revision_ = revision;
        highest_revision_ = revision;
        highest_capture_ = same_epoch ? (std::max)(highest_capture_, capture) : capture;
        failed_capture_floor_ = same_epoch ? (std::max)(failed_capture_floor_, capture) : capture;
        if (!same_epoch)
            identity_fence_.clear();
        pending_.reset();
        acknowledged_.reset();
        waiting_ack_ = true;
        return event;
    }
    SessionKey session_;
    SenderOptions options_;
    mutable std::mutex mutex_;
    std::shared_ptr<const State> acknowledged_, pending_;
    // 仅保留本 epoch 已确认的语义身份；不含观察时戳，不参与帧输出或任何内存读取。
    FrameContext identity_fence_;
    std::optional<Invalidation> invalidation_;
    u64 epoch_ = 0, generation_ = 0, fence_ = 0, highest_revision_ = 0, highest_capture_ = 0;
    u64 invalidated_revision_ = 0, failed_capture_floor_ = 0;
    bool waiting_ack_ = false;
};
template <class Policy> class BasicReceiverSession {
  public:
    using State = typename Policy::State;
    using Update = typename Policy::Update;
    using FrameContext = typename Policy::Context;
    using BaselineOffer = BasicBaselineOffer<State>;
    using Emission = BasicEmission<Update>;

    explicit BasicReceiverSession(SessionKey session, ReceiverOptions options = {})
        : session_(std::move(session)), options_(std::move(options)) {
        session_detail::ValidSession(session_);
        if (!options_.clock)
            throw std::invalid_argument("Synchronization requires a local monotonic clock");
    }
    std::optional<Ack> AcceptBaseline(const BaselineOffer& offer, std::string& error) {
        error.clear();
        if (offer.session != session_ || !offer.state || !offer.generation || !offer.fence || !offer.state->accepted) {
            error = "Invalid baseline session/envelope";
            return std::nullopt;
        }
        {
            std::lock_guard lock(mutex_);
            if (acknowledged_ && offer.state->epoch == epoch_ && offer.generation == generation_ && offer.fence == fence_) {
                if (!Policy::Same(*offer.state, *acknowledged_)) {
                    error = "Baseline token reused for different immutable content";
                    return std::nullopt;
                }
                // 之前已合法安装；重投只重发收据，绝不替换当前状态或修改采样时间。
                return BaselineAck(*acknowledged_);
            }
        }
        const auto entered = options_.clock();
        if (!SampleChronologyValid(offer.state->capture, entered)) {
            error = "Invalid baseline capture chronology";
            return std::nullopt;
        }
        auto owned = std::make_shared<const State>(*offer.state);
        if (!Policy::Validate(*owned, error))
            return std::nullopt;
        Ack ack{session_,        owned->epoch,    offer.generation,       offer.fence,
                owned->revision, owned->revision, owned->capture_sequence};
        if (options_.before_commit)
            options_.before_commit();
        std::lock_guard lock(mutex_);
        if (acknowledged_ && owned->epoch == epoch_ && offer.generation == generation_ && offer.fence == fence_) {
            if (!Policy::Same(*owned, *acknowledged_)) {
                error = "Baseline token reused for different immutable content";
                return std::nullopt;
            }
            return ack;
        }
        if (session_detail::Regressed(owned->epoch, owned->revision, owned->capture_sequence, epoch_, highest_revision_,
                      highest_capture_) ||
            (owned->epoch == epoch_ && (owned->revision <= invalidated_revision_ ||
                                        (failed_capture_floor_ && owned->capture_sequence <= failed_capture_floor_))) ||
            offer.fence <= fence_ || offer.generation <= generation_) {
            error = "Stale baseline fence/revision or failure barrier";
            return std::nullopt;
        }
        if (owned->epoch == epoch_ && owned->revision == highest_revision_ &&
            (!current_ || current_->revision != owned->revision || !Policy::Same(*owned, *current_))) {
            error = "Baseline revision reused for different or unproven immutable content";
            return std::nullopt;
        }
        if (owned->epoch == epoch_ && owned->revision > highest_revision_ && owned->capture_sequence <= highest_capture_) {
            error = "New baseline revision requires a new capture sequence";
            return std::nullopt;
        }
        FrameContext staged_identity;
        if (!Policy::MergeIdentity(owned->epoch == epoch_ ? identity_fence_ : FrameContext{}, owned->context, staged_identity,
                                error))
            return std::nullopt;
        const auto committed = options_.clock();
        if (committed < entered || !SampleChronologyValid(owned->capture, committed)) {
            error = "Invalid baseline chronology before atomic commit";
            return std::nullopt;
        }
        if (owned->epoch != epoch_) {
            invalidated_revision_ = 0;
            failed_capture_floor_ = 0;
        }
        acknowledged_ = owned;
        current_ = std::move(owned);
        rejected_ = false;
        epoch_ = ack.epoch;
        generation_ = ack.generation;
        fence_ = ack.fence;
        highest_revision_ = ack.base_revision;
        highest_capture_ = ack.capture_sequence;
        identity_fence_.swap(staged_identity);
        return ack;
    }
    bool Apply(const SessionKey& session, const Update& update, std::string& error) {
        error.clear();
        if (!update.accepted) {
            error = "Rejected state requires reliable invalidation";
            return false;
        }
        const auto entered = options_.clock();
        if (!SampleChronologyValid(update.capture, entered)) {
            error = "Invalid update capture chronology";
            return false;
        }
        std::shared_ptr<const State> baseline;
        {
            std::lock_guard lock(mutex_);
            if (!Matches(session, update)) {
                error = "Stale update or unmatched ACK baseline";
                return false;
            }
            baseline = acknowledged_;
        }
        State candidate;
        if (!Policy::Apply(*baseline, update, candidate, error))
            return false;
        auto owned = std::make_shared<const State>(std::move(candidate));
        if (options_.before_commit)
            options_.before_commit();
        std::lock_guard lock(mutex_);
        if (acknowledged_ != baseline || !Matches(session, update)) {
            error = "Update lost its baseline before commit";
            return false;
        }
        FrameContext staged_identity;
        if (!Policy::MergeIdentity(identity_fence_, owned->context, staged_identity, error))
            return false;
        const auto committed = options_.clock();
        if (committed < entered || !SampleChronologyValid(owned->capture, committed)) {
            error = "Invalid update chronology before atomic commit";
            return false;
        }
        current_ = std::move(owned);
        rejected_ = false;
        highest_revision_ = update.revision;
        highest_capture_ = update.capture_sequence;
        identity_fence_.swap(staged_identity);
        return true;
    }
    bool Apply(const SessionKey& session, const Emission& emission, std::string& error) {
        if (const auto update = std::get_if<Update>(&emission))
            return Apply(session, *update, error);
        const auto& invalidation = std::get<Invalidation>(emission);
        error.clear();
        if (session != invalidation.session || !Invalidate(invalidation)) {
            error = "Invalid or stale reliable invalidation";
            return false;
        }
        return true;
    }
    bool Invalidate(const Invalidation& event) {
        if (event.session != session_ || !event.epoch || !event.fence || !event.revision)
            return false;
        auto retained = event;
        std::lock_guard lock(mutex_);
        if (invalidation_ && *invalidation_ == event && epoch_ == event.epoch && fence_ == event.fence)
            return true;
        if (session_detail::Regressed(event.epoch, event.revision, event.capture_sequence, epoch_, highest_revision_, highest_capture_) ||
            event.fence <= fence_ || (event.epoch == epoch_ && event.revision <= highest_revision_))
            return false;
        const auto same_epoch = event.epoch == epoch_;
        invalidation_ = std::move(retained);
        epoch_ = event.epoch;
        fence_ = event.fence;
        invalidated_revision_ = event.revision;
        highest_revision_ = event.revision;
        highest_capture_ = same_epoch ? (std::max)(highest_capture_, event.capture_sequence) : event.capture_sequence;
        failed_capture_floor_ =
            same_epoch ? (std::max)(failed_capture_floor_, event.capture_sequence) : event.capture_sequence;
        if (!same_epoch)
            identity_fence_.clear();
        acknowledged_.reset();
        current_.reset();
        rejected_ = true;
        return true;
    }
    std::shared_ptr<const State> Current() const {
        std::lock_guard lock(mutex_);
        return current_ && current_->accepted ? current_ : nullptr;
    }
    ViewStatus Status() const {
        std::lock_guard lock(mutex_);
        if (!current_)
            return rejected_ ? ViewStatus::Rejected : ViewStatus::AwaitingBaseline;
        if (!current_->accepted)
            return ViewStatus::Rejected;
        return ViewStatus::Current;
    }
    std::shared_ptr<const State> Baseline() const {
        std::lock_guard lock(mutex_);
        return acknowledged_;
    }

  private:
    bool SampleChronologyValid(const SampleInterval& sample, Clock::time_point now) const noexcept {
        return sample.started != Clock::time_point{} && sample.started <= sample.finished &&
               (!options_.source_clock_is_local || sample.finished <= now);
    }
    bool Matches(const SessionKey& session, const Update& update) const noexcept {
        return session == session_ && acknowledged_ && update.epoch == epoch_ &&
               update.baseline_generation == generation_ && update.fence == fence_ &&
               update.base_revision == acknowledged_->revision && update.revision > highest_revision_ &&
               update.capture_sequence > highest_capture_;
    }
    Ack BaselineAck(const State& baseline) const {
        return {session_, epoch_, generation_, fence_, baseline.revision, baseline.revision, baseline.capture_sequence};
    }
    SessionKey session_;
    ReceiverOptions options_;
    mutable std::mutex mutex_;
    std::shared_ptr<const State> acknowledged_, current_;
    FrameContext identity_fence_; // 同 epoch 失效/暂时未知不能遗忘；只在更高 epoch 清空。
    std::optional<Invalidation> invalidation_;
    u64 epoch_ = 0, generation_ = 0, fence_ = 0, highest_revision_ = 0, highest_capture_ = 0;
    u64 invalidated_revision_ = 0, failed_capture_floor_ = 0;
    bool rejected_ = false;
};
} // namespace monitor::synchronization
