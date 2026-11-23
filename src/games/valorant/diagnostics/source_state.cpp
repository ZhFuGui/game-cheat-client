#include "source_state.hpp"
#include "application/diagnostics/reader_chain.hpp"
#include "platform/windows/input.hpp"
#include <algorithm>

namespace mousebox::games::valorant
{
using application::Json;
using namespace sources::valorant_reader;
namespace
{
Json optional(std::optional<double> value)
{
    return value ? Json(*value) : Json{};
}
Json vector(std::optional<Vec3> value)
{
    return value ? Json::array({value->x, value->y, value->z}) : Json{};
}
Json camera(const std::optional<Camera> &value)
{
    if (!value)
        return nullptr;
    return {{"position", vector(value->position)},
            {"rotation", vector(value->rotation)},
            {"projection", value->projection},
            {"fov", optional(value->fov)},
            {"aspect", optional(value->aspect)},
            {"constrainAspect", value->constrainAspect ? Json(*value->constrainAspect) : Json{}},
            {"orthoWidth", optional(value->orthoWidth)},
            {"engineTime", optional(value->engineTime)}};
}
constexpr const char *Names[]{"人物位置", "人物骨骼", "相机"};
} // namespace
Json describeSource(const sources::valorant_reader::Observation &observation)
{
    Json channels = Json::array(), streams = Json::array();
    bool connected = false, connecting = false, current = false;
    std::string error;
    for (unsigned i = 0; i < 3; ++i)
    {
        const auto &channel = observation.channels[i];
        connected = connected || channel.connected;
        connecting = connecting || channel.connecting;
        current = current || static_cast<bool>(channel.data);
        if (!channel.error.empty())
            error = channel.error;
        const auto data = channel.data;
        const auto &capture = data ? data->capture : Capture{};
        streams.push_back({{"stream", i + 1},
                           {"connected", channel.connected},
                           {"connecting", channel.connecting},
                           {"current", static_cast<bool>(data)},
                           {"requestedHz", channel.requestedHz},
                           {"subscribedHz", optional(channel.subscribedHz)},
                           {"pending", channel.pending},
                           {"error", channel.error},
                           {"subscriptionError", channel.subscriptionError},
                           {"reader", channel.instance},
                           {"epoch", data ? Json(std::to_string(capture.epoch)) : Json{}},
                           {"capture", data ? Json(std::to_string(capture.sequence)) : Json{}},
                           {"receivedAtMs", data ? Json(channel.receivedAtMs) : Json{}},
                           {"entities", data ? Json(i == 0   ? data->positions.size()
                                                    : i == 1 ? data->skeletons.size()
                                                             : 0)
                                             : Json{}},
                           {"camera", data && i == 2 ? camera(data->camera) : Json{}}});
        if (observation.requested)
            channels.push_back({{"category", i + 1},
                                {"connection", std::to_string(channel.connection)},
                                {"name", std::string("VALORANT · ") + Names[i]},
                                {"status", channel.framesPerSecond ? "waiting-draw" : "waiting-data"},
                                {"averageMs", nullptr},
                                {"uncertaintyMs", nullptr},
                                {"samples", 0},
                                {"receivedFramesPerSecond", channel.framesPerSecond},
                                {"chain", application::describeReaderChain(channel.chain)}});
    }
    const auto &positions = observation.channels[0].data;
    const auto allConnected =
        std::ranges::all_of(observation.channels, [](const auto &channel) { return channel.connected; });
    const auto phase = !observation.requested ? "closed"
                       : connected            ? current && allConnected && error.empty() ? "ready" : "degraded"
                       : connecting           ? "connecting"
                                              : "error";
    const auto message = !observation.requested ? "VALORANT Reader 未连接"
                         : !error.empty()       ? error.c_str()
                         : current              ? "VALORANT Reader 三流独立更新"
                                                : "VALORANT Reader 等待订阅数据";
    Json context = nullptr;
    if (positions)
    {
        const auto &value = positions->context;
        constexpr const char *states[]{"unknown", "absent", "unresolved", "resolved"};
        context = {
            {"map",
             value.map ? Json{{"packagePath", value.map->packagePath}, {"worldName", value.map->worldName}} : Json{}},
            {"local",
             {{"state", states[value.localState]},
              {"entityId", value.localEntity ? Json(std::to_string(*value.localEntity)) : Json{}}}},
            {"referenceTeam", value.referenceBombTeam ? Json(std::to_string(*value.referenceBombTeam)) : Json{}}};
    }
    Json state{{"readerRunning", observation.requested},
               {"readerConnected", connected},
               {"connections", {{"reader", {{"phase", phase}, {"message", message}, {"retryable", true}}}}},
               {"canAim", false},
               {"canCalibrate", false},
               {"aimArmed", false},
               {"aimAutomatic", false},
               {"characters", positions ? Json(positions->positions.size()) : Json{}},
               {"localBodyKnown", positions && positions->context.localState >= 2},
               {"valorant", {{"streams", std::move(streams)}, {"context", std::move(context)}}},
               {"readerMetrics",
                {{"windowMs", 1000},
                 {"uploadBytesPerSecond", observation.upload},
                 {"downloadBytesPerSecond", observation.download},
                 {"gpuCompletionAvailable", false},
                 {"channels", std::move(channels)}}}};
    return {{"state", std::move(state)}, {"algorithm", Json::object()}, {"algorithmSections", Json::array()}};
}
Json readScene(const sources::valorant_reader::Observation &observation, const Json &args)
{
    const auto revision = std::to_string(observation.revision);
    if (args.contains("after") && args.at("after") == revision)
        return {{"unchanged", true}, {"frame", nullptr}};
    Json streams = Json::array();
    for (const auto &channel : observation.channels)
    {
        if (!channel.data)
        {
            streams.push_back(nullptr);
            continue;
        }
        const auto &data = *channel.data;
        if (data.positions.size() > 4096 || data.skeletons.size() > 4096)
            throw std::runtime_error("当前展示数据超过画布预算，Reader完整接收结果仍保留");
        Json entities = Json::array();
        for (const auto &[id, position] : data.positions)
            entities.push_back({{"id", std::to_string(id)}, {"position", vector(position.world)}});
        for (const auto &[id, skeleton] : data.skeletons)
        {
            Json joints = nullptr;
            if (skeleton)
            {
                joints = Json::array();
                for (const auto &joint : *skeleton)
                    joints.push_back(vector(joint));
            }
            entities.push_back({{"id", std::to_string(id)}, {"joints", std::move(joints)}});
        }
        streams.push_back({{"reader", channel.instance},
                           {"epoch", std::to_string(data.capture.epoch)},
                           {"capture", std::to_string(data.capture.sequence)},
                           {"receivedAtMs", channel.receivedAtMs},
                           {"entities", std::move(entities)},
                           {"camera", camera(data.camera)}});
    }
    return {{"unchanged", false}, {"frame", {{"revision", revision}, {"streams", std::move(streams)}}}};
}
} // namespace mousebox::games::valorant
