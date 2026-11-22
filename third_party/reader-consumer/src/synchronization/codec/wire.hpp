#pragma once
#include "synchronization/session.hpp"
#include "synchronization/consumer/session.hpp"
#include <string_view>
#include <utility>

namespace monitor::synchronization::codec {
// 来源映射只属于Reader；消费者时钟独立，不能据握手声明直接相减跨域计数。
inline constexpr unsigned ClockContractVersion = 1;
inline constexpr unsigned ProtocolVersion = 3;
using BusinessKindContract = std::map<EntityKind, std::set<PropertyGroup>>;
BusinessKindContract CanonicalBusinessContract(DataCategory);
enum class ClockKind : unsigned { WindowsQpc = 1, MonotonicCounter = 2 };
struct ClockIdentity {
    std::string domain_id;
    ClockKind kind = ClockKind::MonotonicCounter;
    u64 frequency = 0;
    bool operator==(const ClockIdentity&) const = default;
};
struct ClockMapping {
    std::string domain_id;
    u64 qpc_frequency = 0, mapping_revision = 0, reader_monotonic_anchor_ns = 0, qpc_anchor = 0;
    u64 max_error_ns = 0, valid_from_qpc = 0, valid_until_qpc = 0;
};
struct TransportPolicy {
    unsigned max_tcp_record_bytes = 16777216;
    unsigned heartbeat_interval_ms = 250, heartbeat_timeout_ms = 1000, acknowledgement_timeout_ms = 1000;
    unsigned record_progress_timeout_ms = 1000, invalidation_delivery_timeout_ms = 100;
};
struct KindCapabilities {
    GroupContract contract;
    std::map<EntityKind, std::set<PropertyGroup>> implemented;
};
struct Hello {
    SubscriptionProfile profile = SubscriptionProfile::Consumer;
    SubscriptionFeatures features = 0;
    unsigned protocol_version = ProtocolVersion;
    std::string subscriber_id;
    std::optional<ClockIdentity> consumer_clock;
    unsigned clock_contract_version = ClockContractVersion;
    std::string subscription_id;
    DataCategory category = DataCategory::None;
    double target_hz = 0;
    TransportPolicy transport;
    KindCapabilities kinds;
    u64 nonce = 0;
};
struct HelloAccepted {
    SubscriptionProfile profile = SubscriptionProfile::Consumer;
    SubscriptionFeatures features = 0;
    unsigned protocol_version = ProtocolVersion;
    SessionKey session;
    u64 epoch = 0, nonce = 0;
    TransportPolicy transport;
    ClockMapping clock;
    std::optional<ClockIdentity> consumer_clock;
    unsigned clock_contract_version = ClockContractVersion;
    KindCapabilities kinds;
    BusinessKindContract business_kinds, business_implemented;
    std::string subscription_id;
    DataCategory category = DataCategory::None;
    double target_hz = 0;
    u64 configuration_revision = 0;
};
struct ClockProbe {
    u64 request_id = 0;
    std::optional<u64> consumer_sent_tick;
};
struct ClockProbeReply {
    u64 request_id = 0, epoch = 0, clock_mapping_revision = 0;
    std::optional<u64> consumer_sent_tick, reader_received_qpc, reader_sent_qpc;
};
enum class ResyncReason : unsigned {
    BaselineMissing = 1,
    ValidationFailed = 3,
    EpochChanged = 4,
    TransportReconnected = 5,
    ClockInvalid = 6,
    ReaderFailed = 7
};
struct ResyncRequest {
    u64 request_id = 0, session_id = 0, epoch = 0, generation = 0, last_applied_revision = 0;
    ResyncReason reason = ResyncReason::BaselineMissing;
};
struct InvalidationMessage {
    Invalidation barrier;
    ResyncReason reason = ResyncReason::ReaderFailed;
};
struct InvalidationAck {
    u64 session_id = 0, epoch = 0, fence = 0, revision = 0, capture_sequence_floor = 0;
};
enum class ControlOperation : unsigned { Get = 1, Set = 2 };
enum class ResultStatus : unsigned {
    Ok = 1,
    InvalidArgument,
    Unsupported,
    TooLarge,
    NotReady,
    Conflict,
    InternalError
};
struct CaptureRateRequest {
    u64 request_id = 0;
    ControlOperation operation = ControlOperation::Get;
    std::optional<double> target_hz;
    std::optional<u64> expected_pacing_revision;
};
struct PacingValue {
    u64 configuration_revision = 0, target_interval_ns = 0;
    double target_hz = 0;
};
struct CaptureRateResponse {
    u64 request_id = 0;
    ResultStatus status = ResultStatus::Ok;
    PacingValue adopted;
    double category_target_hz = 0;
    std::string message;
    double classification_target_hz = 1, directory_target_hz = 2;
};
struct Heartbeat {
    u64 nonce = 0, last_applied_revision = 0, pending_generation = 0;
    bool reply = false;
};
struct ErrorResponse {
    u64 request_id = 0;
    ResultStatus status = ResultStatus::InternalError;
    std::string message;
    bool close_connection = false;
};
enum class Direction : unsigned { Reader = 1, Subscriber = 2 };
using Payload = std::variant<Hello, HelloAccepted, BaselineOffer, Ack, ResyncRequest, InvalidationMessage, Update,
                             CaptureRateRequest, CaptureRateResponse, Heartbeat, ErrorResponse, InvalidationAck,
                             ClockProbe, ClockProbeReply, consumer::BaselineOffer, consumer::Update>;
struct ReliableRecord {
    unsigned protocol_version = ProtocolVersion;
    u64 record_sequence = 0, session_id = 0;
    Direction direction = Direction::Reader;
    Payload payload;
};
struct Binding {
    SubscriptionProfile profile = SubscriptionProfile::Consumer;
    SubscriptionFeatures features = 0;
    DataCategory category = DataCategory::None;
    BusinessKindContract business_kinds;
    ClockMapping clock;
    SessionKey session;
    // Consumer默认没有原生身份合同；Debug握手显式安装协商后的合同。
    GroupContract contract;
};
enum class ErrorCode { None, InvalidMessage, TooLarge, InvalidClock, InvalidState };
struct Error {
    ErrorCode code = ErrorCode::None;
    std::string message;
};
class PreparedUpdate;
// 发送正文已完整编码，仅预留正式optional fixed64的8字节；盖戳不分配、不重编码。
class SendStartedAnchor {
  public:
    bool Ready() const noexcept { return size_ != 0; }
    bool Validate(u64 qpc) const noexcept;
    bool Stamp(std::string& bytes, u64 qpc) const noexcept;
    // TCP记录已添加4字节长度前缀后迁移正文偏移；失败不修改对象。
    bool Shift(std::size_t prefix_bytes, std::size_t new_size) noexcept;

  private:
    std::size_t offset_ = 0, size_ = 0;
    u64 earliest_ = 0, valid_from_ = 0, valid_until_ = 0;
    unsigned char tag_ = 0;
    void Initialize(std::size_t offset, std::size_t size, u64 finished, const ClockMapping&, unsigned char tag);
    friend bool PrepareUpdateForSend(const Update&, const Binding&, std::size_t, PreparedUpdate&, SendStartedAnchor&,
                                     Error&);
    friend bool EncodeReliableForSend(const ReliableRecord&, const Binding&, std::size_t, std::string&,
                                      SendStartedAnchor&, Error&);
    friend bool EncodePreparedReliableUpdateForSend(const PreparedUpdate&, u64, std::size_t, std::string&,
                                                    SendStartedAnchor&, Error&);
};
// 仅PrepareUpdate可生成已严格核验的正文；包入可靠TCP记录时避免重复规范化。
class PreparedUpdate {
  public:
    PreparedUpdate() = default;
    PreparedUpdate(const PreparedUpdate&) = delete;
    PreparedUpdate& operator=(const PreparedUpdate&) = delete;
    PreparedUpdate(PreparedUpdate&& other) noexcept
        : session_id_(std::exchange(other.session_id_, 0)), bytes_(std::move(other.bytes_)),
          send_anchor_(std::exchange(other.send_anchor_, {})) {}
    PreparedUpdate& operator=(PreparedUpdate&& other) noexcept {
        if (this != &other) {
            bytes_ = std::move(other.bytes_);
            session_id_ = std::exchange(other.session_id_, 0);
            send_anchor_ = std::exchange(other.send_anchor_, {});
        }
        return *this;
    }
    bool Valid() const noexcept { return session_id_ != 0 && !bytes_.empty(); }
    std::string_view Bytes() const noexcept { return bytes_; }
    std::string TakeBytes() noexcept {
        session_id_ = 0;
        send_anchor_ = {};
        return std::move(bytes_);
    }

  private:
    u64 session_id_ = 0;
    std::string bytes_;
    SendStartedAnchor send_anchor_;
    friend bool PrepareUpdate(const Update&, const Binding&, std::size_t, PreparedUpdate&, Error&);
    friend bool EncodePreparedReliableUpdate(const PreparedUpdate&, u64, std::size_t, std::string&, Error&);
    friend bool PrepareUpdateForSend(const Update&, const Binding&, std::size_t, PreparedUpdate&, SendStartedAnchor&,
                                     Error&);
    friend bool EncodePreparedReliableUpdateForSend(const PreparedUpdate&, u64, std::size_t, std::string&,
                                                    SendStartedAnchor&, Error&);
};
// 以下输出采用局部完整构造后交换：失败不暴露部分 bytes/模型。资源异常交由模块owner安全恢复。
bool EncodeReliable(const ReliableRecord&, const Binding&, std::size_t max_bytes, std::string& bytes, Error&);
bool DecodeReliable(std::string_view bytes, const Binding&, std::size_t max_bytes, ReliableRecord&, Error&);
bool EncodeUpdate(const Update&, const Binding&, std::size_t max_bytes, std::string& bytes, Error&);
bool DecodeUpdate(std::string_view bytes, const Binding&, std::size_t max_bytes, Update&, Error&);
bool PrepareUpdate(const Update&, const Binding&, std::size_t max_body_bytes, PreparedUpdate&, Error&);
// 输出含尚未盖戳的占位；只有SendStartedAnchor::Stamp成功后才可交给发送接口。
bool PrepareUpdateForSend(const Update&, const Binding&, std::size_t max_body_bytes, PreparedUpdate&,
                          SendStartedAnchor&, Error&);
bool EncodeReliableForSend(const ReliableRecord&, const Binding&, std::size_t max_bytes, std::string&,
                           SendStartedAnchor&, Error&);
// 只添加Reader方向的固定Envelope；max_bytes限制完整Envelope，不只限制其中的Update正文。
bool EncodePreparedReliableUpdate(const PreparedUpdate&, u64 record_sequence, std::size_t max_bytes, std::string& bytes,
                                  Error&);
bool EncodePreparedReliableUpdateForSend(const PreparedUpdate&, u64 record_sequence, std::size_t max_bytes,
                                         std::string& bytes, SendStartedAnchor&, Error&);
// 以已核验的锚点作保守整数映射，区间起点向下/终点向上；不刷新原采样时间。
bool ToQpc(const ClockMapping&, Clock::time_point, bool round_up, u64& qpc, Error&);
bool FromQpc(const ClockMapping&, u64 qpc, Clock::time_point&, Error&);
bool ValidateClock(const ClockMapping&, Error&);
bool ValidateClockIdentity(const ClockIdentity&, Error&);
bool ValidatePolicy(const TransportPolicy&, Error&);
} // namespace monitor::synchronization::codec
