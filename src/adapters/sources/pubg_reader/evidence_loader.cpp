#include "adapters/sources/pubg_reader/evidence_loader.hpp"
#include "recorded_codec.hpp"
#include <fstream>
#include <iterator>
#include <stdexcept>
namespace mousebox::reader
{
namespace
{
namespace s = monitor::synchronization;
namespace w = s::codec;
std::string read(const std::filesystem::path &path)
{
    constexpr std::uintmax_t maximum = 16777216;
    if (std::filesystem::file_size(path) > maximum + 4)
        throw std::runtime_error("Evidence file too large");
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("Cannot read evidence");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
w::ReliableRecord decode(std::string_view bytes, const w::Binding &binding)
{
    w::ReliableRecord record;
    w::Error error;
    if (!decodeRecordedReliable(bytes, binding, 16777216, record, error))
        throw std::runtime_error(error.message);
    return record;
}
template <class Receiver, class Baseline, class Update>
ReconstructedEvidence restore(const std::filesystem::path &folder, const w::HelloAccepted &accepted,
                              const w::Binding &binding)
{
    const auto baseRecord = decode(read(folder / "baseline.body.pb"), binding);
    const auto &baseline = std::get<Baseline>(baseRecord.payload);
    if (baseRecord.session_id != accepted.session.session_id || baseRecord.direction != w::Direction::Reader ||
        !baseline.state || baseline.state->Category() != accepted.category)
        throw std::runtime_error("Evidence baseline binding mismatch");
    s::ReceiverOptions options;
    options.source_clock_is_local = false;
    Receiver receiver(accepted.session, options);
    std::string error;
    const auto ack = receiver.AcceptBaseline(baseline, error);
    if (!ack)
        throw std::runtime_error(error);
    const auto packet = read(folder / "ack.packet.pb");
    if (packet.size() < 4)
        throw std::runtime_error("Evidence ACK prefix missing");
    std::uint32_t length = 0;
    for (unsigned i = 0; i < 4; ++i)
        length = (length << 8) | static_cast<unsigned char>(packet[i]);
    if (length != packet.size() - 4)
        throw std::runtime_error("Evidence ACK length mismatch");
    const auto ackRecord = decode(std::string_view(packet).substr(4), binding);
    const auto &raw = std::get<s::Ack>(ackRecord.payload);
    if (ackRecord.direction != w::Direction::Subscriber || raw.session != ack->session || raw.epoch != ack->epoch ||
        raw.generation != ack->generation || raw.fence != ack->fence || raw.base_revision != ack->base_revision ||
        raw.applied_revision != ack->applied_revision || raw.capture_sequence != ack->capture_sequence)
        throw std::runtime_error("Evidence ACK token mismatch");
    const auto updateBytes = read(folder / "update.body.pb");
    if (!updateBytes.empty())
    {
        const auto record = decode(updateBytes, binding);
        if (record.direction != w::Direction::Reader || record.session_id != accepted.session.session_id ||
            !receiver.Apply(accepted.session, std::get<Update>(record.payload), error))
            throw std::runtime_error("Evidence update: " + error);
    }
    const auto state = receiver.Current();
    if (!state)
        throw std::runtime_error("Evidence has no validated current state");
    return {binding, state, projectControlFrame(*state, binding)};
}
} // namespace
ReconstructedEvidence loadEvidence(const std::filesystem::path &folder)
{
    const auto acceptedRecord = decode(read(folder / "accepted.body.pb"), {});
    const auto &accepted = std::get<w::HelloAccepted>(acceptedRecord.payload);
    if (acceptedRecord.direction != w::Direction::Reader || acceptedRecord.session_id != accepted.session.session_id)
        throw std::runtime_error("Evidence Accepted binding mismatch");
    w::Binding binding;
    binding.profile = accepted.profile;
    binding.features = accepted.features;
    binding.category = accepted.category;
    binding.business_kinds = accepted.business_kinds;
    binding.clock = accepted.clock;
    binding.session = accepted.session;
    binding.contract = accepted.kinds.contract;
    if (accepted.profile == monitor::SubscriptionProfile::Consumer)
        return restore<s::consumer::ReceiverSession, s::consumer::BaselineOffer, s::consumer::Update>(folder, accepted,
                                                                                                      binding);
    return restore<s::ReceiverSession, s::BaselineOffer, s::Update>(folder, accepted, binding);
}
} // namespace mousebox::reader
