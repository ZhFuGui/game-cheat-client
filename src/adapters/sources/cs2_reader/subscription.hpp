#pragma once
#include "codec.hpp"
#include <cmath>
#include <stdexcept>

namespace mousebox::sources::cs2_reader
{
inline constexpr double MaximumSubscriptionHz = 300;

// Owned by Client under its state mutex. Only the socket thread assigns request
// IDs or consumes replies; UI edits coalesce without interrupting a record.
class Subscription
{
    struct Pending
    {
        std::uint64_t id, lastProgressAtMs;
        double hz;
    };
    double requested_ = 30;
    std::uint64_t revision_ = 0;
    std::optional<std::uint64_t> submitted_;
    std::optional<Pending> pending_;
    std::optional<double> confirmed_, capture_;
    std::string error_;

  public:
    void set(double hz)
    {
        if (!std::isfinite(hz) || hz < 1 || hz > MaximumSubscriptionHz)
            throw std::invalid_argument("CS2订阅频率须为1–300Hz");
        if (hz == requested_)
            return;
        requested_ = hz;
        ++revision_;
        error_.clear();
    }
    void reset()
    {
        submitted_.reset();
        pending_.reset();
        confirmed_.reset();
        capture_.reset();
        error_.clear();
    }
    bool needsRequest() const
    {
        return !pending_ && submitted_ != revision_;
    }
    double begin(std::uint64_t id, std::uint64_t now)
    {
        if (!id || !needsRequest())
            throw std::logic_error("CS2 subscription request is not ready");
        pending_ = Pending{id, now, requested_};
        submitted_ = revision_;
        return requested_;
    }
    void confirm(const Envelope &reply)
    {
        if (!pending_ || reply.request != pending_->id || !std::isfinite(reply.publishHz) ||
            reply.publishHz != pending_->hz || !std::isfinite(reply.captureHz) || reply.captureHz < 1 ||
            reply.captureHz > MaximumSubscriptionHz || reply.publishHz > reply.captureHz)
            throw std::runtime_error("CS2 subscription acknowledgement mismatch");
        confirmed_ = reply.publishHz;
        capture_ = reply.captureHz;
        pending_.reset();
        error_.clear();
    }
    bool reject(const Envelope &reply)
    {
        if (!pending_ || reply.request != pending_->id)
            return false;
        pending_.reset();
        error_ = reply.error;
        return true;
    }
    bool expired(std::uint64_t now) const
    {
        return pending_ && now >= pending_->lastProgressAtMs && now - pending_->lastProgressAtMs > 3000;
    }
    void receiveProgress(std::uint64_t now)
    {
        if (pending_)
            pending_->lastProgressAtMs = now;
    }
    bool waiting() const
    {
        return pending_.has_value() || needsRequest();
    }
    double requested() const
    {
        return requested_;
    }
    std::optional<double> confirmed() const
    {
        return confirmed_;
    }
    std::optional<double> capture() const
    {
        return capture_;
    }
    const std::string &error() const
    {
        return error_;
    }
};
} // namespace mousebox::sources::cs2_reader
