#include "inspection.hpp"
#include "adapters/sources/inspection/protobuf_json.hpp"
#include "synchronization/codec/consumer_codec.hpp"
namespace mousebox::reader
{
application::ports::SourcePacket inspectConsumer(const Session *session)
{
    if (!session || !session->current())
        return {};
    const auto state = session->current();
    const auto binding = session->binding();
    const bool living = state->category == monitor::DataCategory::Living;
    const auto identity = binding.session.stream_id + ":" + std::to_string(binding.session.session_id) + ":" +
                          std::to_string(state->epoch);
    return {living ? "living" : "local",
            living ? "PUBG Living" : "PUBG Local",
            identity,
            identity + ":" + std::to_string(state->revision) + ":" + std::to_string(state->capture_sequence),
            "materialized",
            [state, binding] {
                // The codec's temporary baseline container only exposes the SDK's
                // already merged current values. Its synthetic ACK tokens are removed.
                const consumer::BaselineOffer view{binding.session, 1, 1, state};
                const auto object = wire::detail::ConsumerBaseline(view, binding);
                const auto bytes =
                    wire::detail::EncodeMessage(wire::detail::ConsumerBaselineSchema, object, 16 * 1024 * 1024);
                auto json = sources::inspection::protobufJson(bytes, "pubg.reader.synchronization.v3.ConsumerBaseline");
                json.erase("generation");
                json.erase("fence");
                return json;
            }};
}
} // namespace mousebox::reader
