#include "synchronization/codec/detail.hpp"
#include <algorithm>
#include <limits>

namespace monitor::synchronization::codec {
namespace {
template <class F> bool Checked(F&& work, Error& error) {
    try {
        work();
        error = {};
        return true;
    } catch (const detail::Failure& failure) {
        error = {failure.code, failure.what()};
        return false;
    } catch (const std::bad_variant_access&) {
        error = {ErrorCode::InvalidMessage, "Invalid normalized field type"};
        return false;
    } catch (const std::out_of_range&) {
        error = {ErrorCode::InvalidMessage, "Missing normalized field"};
        return false;
    } catch (const std::invalid_argument& problem) {
        error = {ErrorCode::InvalidMessage, problem.what()};
        return false;
    }
}
void AppendVarint(std::string& bytes, u64 value) {
    while (value >= 128) {
        bytes.push_back(static_cast<char>((value & 127) | 128));
        value >>= 7;
    }
    bytes.push_back(static_cast<char>(value));
}
u64 ClockTolerance(const ClockMapping& clock) noexcept {
    // max_error_ns已校验<=1ms；拆商避免frequency*ns溢出，另留一个整数向上舍入tick。
    const auto whole = clock.qpc_frequency / 1000000000 * clock.max_error_ns;
    const auto remainder = clock.qpc_frequency % 1000000000 * clock.max_error_ns;
    return whole + remainder / 1000000000 + (remainder % 1000000000 != 0 ? 1 : 0) + 1;
}
u64 FinishedQpc(const SampleInterval& capture, const Binding& binding) {
    u64 finished = 0;
    Error error;
    detail::Require(ToQpc(binding.clock, capture.finished, true, finished, error), error.message,
                    ErrorCode::InvalidClock);
    return finished;
}
void Placeholder(const std::string& bytes, unsigned char tag) {
    detail::Require(bytes.size() >= 9 && static_cast<unsigned char>(bytes[bytes.size() - 9]) == tag &&
                        std::all_of(bytes.end() - 8, bytes.end(), [](char byte) { return byte == 0; }),
                    "Send anchor is not the final reserved fixed64 field");
}
} // namespace
namespace detail {
void ValidateSendStarted(u64 qpc, u64 finished, const Binding& binding) {
    Error error;
    Require(ValidateClock(binding.clock, error), error.message, ErrorCode::InvalidClock);
    const auto tolerance = ClockTolerance(binding.clock);
    Require(qpc && qpc >= binding.clock.valid_from_qpc && qpc <= binding.clock.valid_until_qpc &&
                (qpc >= finished || finished - qpc <= tolerance),
            "Invalid send-started clock/window/chronology", ErrorCode::InvalidClock);
}
} // namespace detail
void SendStartedAnchor::Initialize(std::size_t offset, std::size_t size, u64 finished, const ClockMapping& clock,
                                   unsigned char tag) {
    Error error;
    detail::Require(ValidateClock(clock, error), error.message, ErrorCode::InvalidClock);
    detail::Require(offset && offset <= size && size - offset == 8, "Invalid send anchor position");
    const auto tolerance = ClockTolerance(clock);
    offset_ = offset;
    size_ = size;
    earliest_ = finished > tolerance ? finished - tolerance : 1;
    valid_from_ = clock.valid_from_qpc;
    valid_until_ = clock.valid_until_qpc;
    tag_ = tag;
}
bool SendStartedAnchor::Validate(u64 qpc) const noexcept {
    return Ready() && qpc && qpc >= earliest_ && qpc >= valid_from_ && qpc <= valid_until_;
}
bool SendStartedAnchor::Stamp(std::string& bytes, u64 qpc) const noexcept {
    if (!Validate(qpc) || bytes.size() != size_ || !offset_ || offset_ > bytes.size() || bytes.size() - offset_ < 8 ||
        static_cast<unsigned char>(bytes[offset_ - 1]) != tag_)
        return false;
    for (unsigned byte = 0; byte < 8; ++byte)
        bytes[offset_ + byte] = static_cast<char>((qpc >> (byte * 8)) & 0xff);
    return true;
}
bool SendStartedAnchor::Shift(std::size_t prefix, std::size_t new_size) noexcept {
    if (!Ready() || prefix > std::numeric_limits<std::size_t>::max() - size_ || size_ + prefix != new_size)
        return false;
    offset_ += prefix;
    size_ = new_size;
    return true;
}
bool EncodeReliable(const ReliableRecord& record, const Binding& binding, std::size_t maximum, std::string& bytes,
                    Error& error) {
    return Checked(
        [&] {
            auto result =
                detail::EncodeMessage(detail::ReliableEnvelopeSchema, detail::Record(record, binding), maximum);
            bytes.swap(result);
        },
        error);
}
bool DecodeReliable(std::string_view bytes, const Binding& binding, std::size_t maximum, ReliableRecord& output,
                    Error& error) {
    return Checked(
        [&] {
            auto result =
                detail::ReadRecord(detail::DecodeMessage(detail::ReliableEnvelopeSchema, bytes, maximum), binding);
            output = std::move(result);
        },
        error);
}
bool EncodeUpdate(const Update& update, const Binding& binding, std::size_t maximum, std::string& bytes, Error& error) {
    return Checked(
        [&] {
            auto result = detail::EncodeMessage(detail::UpdateSchema, detail::Delta(update, binding), maximum);
            bytes.swap(result);
        },
        error);
}
bool PrepareUpdate(const Update& update, const Binding& binding, std::size_t maximum, PreparedUpdate& output,
                   Error& error) {
    return Checked(
        [&] {
            PreparedUpdate result;
            result.bytes_ = detail::EncodeMessage(detail::UpdateSchema, detail::Delta(update, binding), maximum);
            result.session_id_ = binding.session.session_id;
            output = std::move(result);
        },
        error);
}
bool PrepareUpdateForSend(const Update& update, const Binding& binding, std::size_t maximum, PreparedUpdate& output,
                          SendStartedAnchor& anchor, Error& error) {
    return Checked(
        [&] {
            detail::Require(!update.send_started_qpc, "Prepared send requires a new send-anchor attempt");
            auto object = detail::Delta(update, binding);
            object.insert_or_assign(12, Value(u64(0))); // 仅此尚未发送buffer允许占位0。
            PreparedUpdate result;
            result.bytes_ = detail::EncodeMessage(detail::UpdateSchema, object, maximum);
            Placeholder(result.bytes_, 0x61);
            result.session_id_ = binding.session.session_id;
            result.send_anchor_.Initialize(result.bytes_.size() - 8, result.bytes_.size(),
                                           FinishedQpc(update.capture, binding), binding.clock, 0x61);
            const auto prepared_anchor = result.send_anchor_;
            output = std::move(result);
            anchor = prepared_anchor;
        },
        error);
}
bool EncodeReliableForSend(const ReliableRecord& record, const Binding& binding, std::size_t maximum,
                           std::string& bytes, SendStartedAnchor& anchor, Error& error) {
    return Checked(
        [&] {
            auto object = detail::Record(record, binding);
            SendStartedAnchor prepared_anchor;
            u64 finished = 0;
            unsigned char tag = 0;
            if (const auto* baseline = std::get_if<BaselineOffer>(&record.payload)) {
                detail::Require(!baseline->send_started_qpc, "Prepared baseline requires a new send-anchor attempt");
                auto& body = std::get<detail::Object>(object.at(7).data);
                body.insert_or_assign(9, Value(u64(0)));
                finished = FinishedQpc(baseline->state->capture, binding);
                tag = 0x49;
            } else if (const auto* update = std::get_if<Update>(&record.payload)) {
                detail::Require(!update->send_started_qpc, "Prepared update requires a new send-anchor attempt");
                auto& body = std::get<detail::Object>(object.at(11).data);
                body.insert_or_assign(12, Value(u64(0)));
                finished = FinishedQpc(update->capture, binding);
                tag = 0x61;
            } else if (const auto* baseline = std::get_if<consumer::BaselineOffer>(&record.payload)) {
                detail::Require(!baseline->send_started_qpc, "Consumer baseline requires a new send-anchor attempt");
                auto& body = std::get<detail::Object>(object.at(19).data);
                body.insert_or_assign(9, Value(u64(0)));
                finished = FinishedQpc(baseline->state->capture, binding);
                tag = 0x49;
            } else if (const auto* update = std::get_if<consumer::Update>(&record.payload)) {
                detail::Require(!update->send_started_qpc, "Consumer update requires a new send-anchor attempt");
                auto& body = std::get<detail::Object>(object.at(20).data);
                body.insert_or_assign(15, Value(u64(0)));
                finished = FinishedQpc(update->capture, binding);
                tag = 0x79;
            } else if (const auto* reply = std::get_if<ClockProbeReply>(&record.payload)) {
                detail::Require(!reply->reader_sent_qpc, "Clock reply requires a new send-anchor attempt");
                auto& body = std::get<detail::Object>(object.at(18).data);
                body.insert_or_assign(6, Value(u64(0)));
                // t1和t2直接来自QPC，不经过steady_clock映射舍入；保守下界也不得早于t1。
                finished = *reply->reader_received_qpc;
                tag = 0x31;
            }
            auto result = detail::EncodeMessage(detail::ReliableEnvelopeSchema, object, maximum);
            if (tag) {
                Placeholder(result, tag);
                prepared_anchor.Initialize(result.size() - 8, result.size(), finished, binding.clock, tag);
                if (std::holds_alternative<ClockProbeReply>(record.payload))
                    prepared_anchor.earliest_ = finished; // 原生QPC无需内部映射的舍入容差。
            }
            bytes.swap(result);
            anchor = prepared_anchor;
        },
        error);
}
bool EncodePreparedReliableUpdate(const PreparedUpdate& update, u64 sequence, std::size_t maximum, std::string& bytes,
                                  Error& error) {
    return Checked(
        [&] {
            detail::Require(update.Valid() && sequence, "Invalid prepared update/record sequence");
            detail::Require(maximum && maximum <= 16777216, "Invalid protobuf size limit", ErrorCode::TooLarge);
            // 与Record()固定字段及protobuf编码顺序一致；正文已经通过Delta和UpdateSchema完整核验。
            auto result = detail::EncodeMessage(detail::ReliableEnvelopeSchema,
                                                {{1, Value(u64(ProtocolVersion))},
                                                 {2, Value(sequence)},
                                                 {3, Value(u64(Direction::Reader))},
                                                 {4, Value(update.session_id_)}},
                                                maximum);
            AppendVarint(result, (u64(11) << 3) | 2); // ReliableEnvelope.oversized_update
            AppendVarint(result, static_cast<u64>(update.bytes_.size()));
            detail::Require(result.size() <= maximum && update.bytes_.size() <= maximum - result.size(),
                            "Protobuf message exceeds size limit", ErrorCode::TooLarge);
            result.reserve(result.size() + update.bytes_.size()); // 上面的减法检查也排除了size_t加法溢出。
            result.append(update.bytes_);
            bytes.swap(result);
        },
        error);
}
bool EncodePreparedReliableUpdateForSend(const PreparedUpdate& update, u64 sequence, std::size_t maximum,
                                         std::string& bytes, SendStartedAnchor& anchor, Error& error) {
    return Checked(
        [&] {
            detail::Require(update.send_anchor_.Ready(), "Prepared update has no send-anchor reservation");
            std::string result;
            Error wrapped;
            detail::Require(EncodePreparedReliableUpdate(update, sequence, maximum, result, wrapped), wrapped.message,
                            wrapped.code);
            auto prepared_anchor = update.send_anchor_;
            detail::Require(prepared_anchor.Shift(result.size() - update.bytes_.size(), result.size()),
                            "Cannot shift prepared send anchor");
            Placeholder(result, 0x61);
            bytes.swap(result);
            anchor = prepared_anchor;
        },
        error);
}
bool DecodeUpdate(std::string_view bytes, const Binding& binding, std::size_t maximum, Update& output, Error& error) {
    return Checked(
        [&] {
            auto result = detail::ReadDelta(detail::DecodeMessage(detail::UpdateSchema, bytes, maximum), binding);
            output = std::move(result);
        },
        error);
}
} // namespace monitor::synchronization::codec
