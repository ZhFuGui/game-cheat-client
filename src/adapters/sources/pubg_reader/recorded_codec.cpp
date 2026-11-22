#include "recorded_codec.hpp"
#include "synchronization/codec/detail.hpp"

namespace mousebox::reader
{
bool decodeRecordedReliable(std::string_view bytes, const monitor::synchronization::codec::Binding &binding,
                            std::size_t maximum, monitor::synchronization::codec::ReliableRecord &record,
                            monitor::synchronization::codec::Error &error)
{
    namespace w = monitor::synchronization::codec;
    namespace d = w::detail;
    namespace s = monitor::synchronization;
    try
    {
        auto object = d::DecodeMessage(d::ReliableEnvelopeSchema, bytes, maximum);
        if (d::UInt(object, 1) != 2)
            return w::DecodeReliable(bytes, binding, maximum, record, error);
        object[1] = s::Value(monitor::u64(w::ProtocolVersion));
        // v2 carried the rich verified model. Its explicit replay profile is
        // Debug; never manufacture a Consumer entity from native addresses.
        if (object.contains(6))
        {
            auto &accepted = std::get<s::Value::Object>(object.at(6).data);
            accepted[1] = s::Value(monitor::u64(w::ProtocolVersion));
            accepted[15] = s::Value(monitor::u64(monitor::SubscriptionProfile::Debug));
        }
        if (object.contains(5))
            throw std::runtime_error("Recorded Hello is not an applied evidence record");
        auto offlineBinding = binding;
        offlineBinding.profile = monitor::SubscriptionProfile::Debug;
        offlineBinding.features = 0;
        offlineBinding.business_kinds.clear();
        const auto converted = d::EncodeMessage(d::ReliableEnvelopeSchema, object, maximum);
        return w::DecodeReliable(converted, offlineBinding, maximum, record, error);
    }
    catch (const std::exception &exception)
    {
        error = {w::ErrorCode::InvalidMessage, exception.what()};
        return false;
    }
}
} // namespace mousebox::reader
