#include "codec.hpp"
#include "context_decode.hpp"
#include <algorithm>
#include <bit>
#include <set>

namespace mousebox::sources::valorant_reader
{
using namespace wire;
namespace
{
bool boolean(const Fields &fields, unsigned id)
{
    const auto value = integer(fields, id);
    require(value <= 1, "VALORANT invalid boolean");
    return value != 0;
}
unsigned body(const Fields &fields, unsigned first, unsigned last)
{
    unsigned selected = 0;
    for (unsigned id = first; id <= last; ++id)
        if (!many(fields, id).empty())
        {
            require(!selected && one(fields, id, 2), "VALORANT multiple oneof bodies");
            selected = id;
        }
    return selected;
}
bool identity(std::string_view value)
{
    return value.size() == 16 && std::ranges::any_of(value, [](char c) { return c != 0; });
}
std::vector<std::uint64_t> integers(const Fields &fields, unsigned id)
{
    std::vector<std::uint64_t> result;
    for (const auto &field : many(fields, id))
        if (field.type == 0)
            result.push_back(field.value);
        else
        {
            require(field.type == 2, "VALORANT wrong repeated integer wire type");
            protocol::protobuf::Reader reader(field.bytes);
            while (!reader.empty())
                result.push_back(reader.varint());
        }
    return result;
}
std::optional<Vec3> vector(const Fields &fields, unsigned id)
{
    const auto field = one(fields, id, 2);
    if (!field)
        return {};
    const auto values = parse(field->bytes);
    return Vec3{optionalReal(values, 1, 1).value_or(0), optionalReal(values, 2, 1).value_or(0),
                optionalReal(values, 3, 1).value_or(0)};
}
std::optional<Camera> camera(std::string_view input)
{
    const auto state = parse(input);
    const auto value = one(state, 1, 2);
    if (!value)
        return {};
    const auto fields = parse(value->bytes);
    Camera result;
    result.position = vector(fields, 1);
    result.rotation = vector(fields, 2);
    const auto projection = integer(fields, 3);
    require(projection <= UINT32_MAX, "VALORANT projection overflow");
    result.projection = projection <= 2 ? static_cast<unsigned>(projection) : 0;
    result.fov = optionalReal(fields, 4, 5);
    result.aspect = optionalReal(fields, 5, 5);
    if (one(fields, 6, 0))
        result.constrainAspect = boolean(fields, 6);
    result.orthoWidth = optionalReal(fields, 7, 5);
    result.engineTime = optionalReal(fields, 8, 5);
    return result;
}
std::optional<Skeleton> skeleton(const Fields &entity)
{
    const auto value = one(entity, 2, 2);
    if (!value)
        return {};
    const auto fields = parse(value->bytes);
    require(integer(fields, 1) == 1, "VALORANT unknown skeleton layout");
    const auto mask = integer(fields, 2, 5);
    require(mask && !(mask >> 22), "VALORANT invalid skeleton presence mask");
    std::vector<double> xyz;
    for (const auto &field : many(fields, 3))
    {
        if (field.type == 1)
            xyz.push_back(real(field));
        else
        {
            require(field.type == 2 && field.bytes.size() % 8 == 0, "VALORANT invalid packed coordinates");
            protocol::protobuf::Reader reader(field.bytes);
            while (!reader.empty())
                xyz.push_back(real({1, reader.fixed64(), {}}));
        }
        require(xyz.size() <= 66, "VALORANT skeleton coordinate overflow");
    }
    require(xyz.size() == 3u * std::popcount(mask), "VALORANT skeleton mask/coordinates disagree");
    Skeleton result;
    std::size_t offset = 0;
    for (unsigned joint = 0; joint < result.size(); ++joint)
        if (mask & (std::uint64_t{1} << joint))
        {
            result[joint] = Vec3{xyz[offset], xyz[offset + 1], xyz[offset + 2]};
            offset += 3;
        }
    return result;
}
} // namespace
Envelope decode(std::string_view input)
{
    const auto fields = parse(input);
    require(integer(fields, 1) == 1, "VALORANT Reader protocol version mismatch");
    Envelope out;
    out.event = integer(fields, 2);
    require(out.event > 0, "VALORANT missing event sequence");
    out.sent = optionalInteger(fields, 3, 1);
    out.mapping = optionalInteger(fields, 4);
    require(!out.mapping || *out.mapping > 0, "VALORANT invalid mapping revision");
    out.body = body(fields, 10, 23);
    require(out.body == 10 || out.body == 13 || out.body == 14 || out.body == 16 || out.body == 17 || out.body == 19 ||
                out.body == 21 || out.body == 23,
            "VALORANT unexpected server message direction or projection");
    out.payload = parse(bytes(fields, out.body));
    return out;
}
Hello decodeHello(const Envelope &envelope)
{
    require(envelope.body == 10, "VALORANT expected Hello");
    const auto &fields = envelope.payload;
    require(integer(fields, 1) == 1 && hex(bytes(fields, 2)) == SchemaHash, "VALORANT Reader schema mismatch");
    const auto instance = bytes(fields, 3);
    const auto clock = parse(bytes(fields, 4));
    const auto domain = bytes(clock, 1);
    Hello result;
    result.frequency = integer(clock, 2, 1);
    result.clockRevision = integer(clock, 3);
    require(identity(instance) && identity(domain) && result.frequency && result.clockRevision,
            "VALORANT invalid Reader/clock identity");
    result.instance = hex(instance);
    result.domain = hex(domain);
    for (auto stream : integers(fields, 5))
    {
        require(stream >= 1 && stream <= 3 && !result.streams[stream], "VALORANT invalid stream capabilities");
        result.streams[stream] = true;
    }
    require(boolean(fields, 6), "VALORANT baseline/delta capability unavailable");
    result.clockExchange = boolean(fields, 7);
    result.maximumHz = optionalReal(fields, 8, 1).value_or(0);
    require(result.maximumHz >= 1 && result.maximumHz <= 300, "VALORANT invalid maximum request rate");
    return result;
}
bool applyUpdate(const Envelope &envelope, unsigned stream, std::uint64_t clockRevision, StreamState &state)
{
    require(envelope.body == 14 && stream >= 1 && stream <= 3, "VALORANT expected consumer update");
    const auto &update = envelope.payload;
    const auto metadata = parse(bytes(update, 1));
    Capture capture;
    capture.epoch = integer(metadata, 1);
    capture.sequence = integer(metadata, 2);
    capture.configuration = integer(metadata, 3);
    capture.periodNs = integer(metadata, 4);
    capture.clockRevision = integer(metadata, 5);
    require(capture.epoch && capture.sequence && capture.configuration && capture.periodNs &&
                capture.clockRevision == clockRevision,
            "VALORANT invalid capture identity");
    const auto interval = parse(bytes(metadata, 6));
    capture.started = optionalInteger(interval, 1, 1);
    capture.readFinished = optionalInteger(interval, 2, 1);
    capture.finished = optionalInteger(interval, 3, 1);
    require((!capture.started || !capture.readFinished || *capture.started <= *capture.readFinished) &&
                (!capture.readFinished || !capture.finished || *capture.readFinished <= *capture.finished) &&
                (!capture.started || !capture.finished || *capture.started <= *capture.finished),
            "VALORANT capture interval reversed");
    if (state.capture.epoch)
        require(capture.epoch >= state.capture.epoch &&
                    (capture.epoch != state.capture.epoch || capture.sequence > state.capture.sequence),
                "VALORANT stale capture sequence");
    const auto kind = body(update, 2, 3);
    require(kind != 0, "VALORANT missing update body");
    const auto payload = parse(bytes(update, kind));
    const bool full = kind == 2;
    if (!full &&
        (!state.baseline || state.capture.epoch != capture.epoch || integer(payload, 1) != state.capture.sequence))
        return false;
    auto next = full ? StreamState{} : state;
    next.stream = stream;
    next.capture = capture;
    next.baseline = true;
    const auto category = body(payload, full ? 1 : 2, full ? 3 : 4);
    require((!full && !category) || category == stream + (full ? 0 : 1), "VALORANT cross-stream update");
    if (category)
    {
        if (stream == 3)
            next.camera = camera(bytes(payload, category));
        else
        {
            const auto changes = parse(bytes(payload, category));
            if (stream == 1)
            {
                const auto context = one(changes, 3, 2);
                require(!full || context, "VALORANT full positions snapshot lacks context");
                if (context)
                    next.context = decodeContext(context->bytes);
            }
            std::set<std::uint64_t> seen;
            for (const auto &field : many(changes, 1))
            {
                require(field.type == 2, "VALORANT invalid entity record");
                const auto entity = parse(field.bytes);
                const auto id = integer(entity, 1);
                require(id && seen.insert(id).second, "VALORANT duplicate entity identity");
                if (stream == 1)
                {
                    const auto relationship = integer(entity, 4);
                    require(relationship <= UINT32_MAX, "VALORANT relationship overflow");
                    next.positions[id] = Position{vector(entity, 2), bombTeam(entity, 3),
                                                  relationship <= 3 ? static_cast<unsigned>(relationship) : 0};
                }
                else
                    next.skeletons[id] = skeleton(entity);
            }
            if (!full)
                for (auto id : integers(changes, 2))
                {
                    require(id && seen.insert(id).second, "VALORANT duplicate/intersecting removed identity");
                    next.positions.erase(id);
                    next.skeletons.erase(id);
                }
        }
    }
    state = std::move(next);
    return true;
}
std::string schemaBytes()
{
    std::string result;
    const auto digit = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };
    for (std::size_t i = 0; i < SchemaHash.size(); i += 2)
        result += static_cast<char>((digit(SchemaHash[i]) << 4) | digit(SchemaHash[i + 1]));
    return result;
}
std::string request(unsigned bodyId, std::string payload)
{
    std::string out;
    number(out, 1, 1);
    block(out, bodyId, payload);
    require(out.size() <= 4096, "VALORANT control record exceeds limit");
    std::string packet;
    for (int shift = 24; shift >= 0; shift -= 8)
        packet += static_cast<char>(out.size() >> shift);
    return packet + out;
}
std::string subscribe(std::uint64_t id, unsigned stream, double hz)
{
    std::string value;
    number(value, 1, id);
    number(value, 2, 1);
    block(value, 3, schemaBytes());
    number(value, 4, stream);
    number(value, 5, std::bit_cast<std::uint64_t>(hz), 1);
    number(value, 6, 1);
    return request(11, std::move(value));
}
std::string rate(std::uint64_t id, std::uint64_t revision, double hz)
{
    std::string value;
    number(value, 1, id);
    number(value, 2, revision);
    number(value, 3, std::bit_cast<std::uint64_t>(hz), 1);
    return request(12, std::move(value));
}
} // namespace mousebox::sources::valorant_reader
