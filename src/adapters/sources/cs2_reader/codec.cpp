#include "codec.hpp"
#include "snapshot_decode.hpp"
#include <cs2_reader/v2/schema.hpp>
#include <bit>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace mousebox::sources::cs2_reader
{
using namespace detail;
using games::cs2::Snapshot;
const std::string_view SchemaHash = ::cs2_reader::v2::schema_sha256_hex;
namespace
{
constexpr auto Version = ::cs2_reader::v2::protocol_version;
void putVar(std::string &b, std::uint64_t n)
{
    while (n >= 128)
    {
        b += static_cast<char>((n & 127) | 128);
        n >>= 7;
    }
    b += static_cast<char>(n);
}
void putNumber(std::string &b, unsigned id, std::uint64_t n, unsigned w = 0)
{
    putVar(b, (std::uint64_t(id) << 3) | w);
    if (w == 0)
        putVar(b, n);
    else
        for (unsigned i = 0; i < 8; ++i)
            b += static_cast<char>(n >> (i * 8));
}
void putBytes(std::string &b, unsigned id, std::string_view s)
{
    putVar(b, (std::uint64_t(id) << 3) | 2);
    putVar(b, s.size());
    b += s;
}
std::string_view schemaBytes()
{
    const auto &digest = ::cs2_reader::v2::schema_sha256;
    return {reinterpret_cast<const char *>(digest.data()), digest.size()};
}
std::string reasonText(unsigned code)
{
    if (!code)
        return {};
    // The Reader v2 registry assigns 1–10 to status reasons and 11–14 to
    // subscription rejection. Unknown positive codes remain visible.
    constexpr std::string_view descriptions[]{
        "采集配置与当前来源不兼容", "暂时无法发现目标",     "暂时无法绑定目标",
        "采集所需上下文尚未就绪",   "采集工作线程暂不可用", "目标已退出或已被替换",
        "采集上下文变化",           "当批采集暂时失败",     "数据编码失败",
        "此前采集数据已失效",       "订阅请求序号陈旧",     "订阅协议版本不匹配",
        "订阅协议摘要不匹配",       "请求频率超出支持范围",
    };
    auto label = std::to_string(code);
    if (label.size() < 3)
        label.insert(0, 3 - label.size(), '0');
    label.insert(0, "R");
    if (code <= std::size(descriptions))
    {
        label += " · ";
        label += descriptions[code - 1];
    }
    return label;
}
std::string envelope(unsigned body, std::string payload)
{
    std::string b;
    putNumber(b, 1, Version);
    putBytes(b, body, payload);
    return record(std::move(b));
}
} // namespace
Envelope decode(std::string_view b)
{
    auto f = fields(b);
    const auto serverVersion = number(f, 1);
    if (serverVersion != Version)
        throw std::runtime_error("CS2 Reader 协议版本不匹配（需要 v" + std::to_string(Version) + "，收到 v" +
                                 std::to_string(serverVersion) + "）");
    Envelope out;
    auto instance = bytes(f, 2);
    require(instance.size() == 16, "CS2 server instance invalid");
    out.instance = hex(instance);
    out.event = number(f, 3, 1);
    require(out.event > 0, "CS2 event sequence invalid");
    out.sent = optional64(f, 4);
    const Field *value = nullptr;
    for (const auto &[id, values] : f)
        if (id >= 10 && id <= 18)
        {
            require(!value && values.size() == 1 && values.front().wire == 2, "CS2 envelope body invalid");
            value = &values.front();
            out.body = id;
        }
    require(value && out.body != 11 && out.body != 15, "CS2 server direction/body invalid");
    auto g = fields(value->bytes);
    if (out.body == 10)
    {
        require(number(g, 1) == Version, "CS2 Reader Hello 与外层协议版本不一致");
        require(bytes(g, 2) == schemaBytes(), "CS2 Reader 协议摘要不匹配");
        auto [d, q] = decodeClock(bytes(g, 4));
        out.clockDomain = d;
        out.frequency = q;
        auto rate = one(g, 5, 1);
        require(rate, "CS2 capture rate missing");
        out.captureHz = real(*rate);
        require(out.captureHz >= 1 && out.captureHz <= 10000, "CS2 capture rate invalid");
        const auto backend = number(g, 3);
        require(backend == 1 || backend == 2, "CS2 invalid backend capability");
        out.full = optionalBool(g, 6).value_or(false);
        out.clockExchange = optionalBool(g, 7).value_or(false);
        out.independentGroupTimestamps = optionalBool(g, 8).value_or(false);
        out.verifiedBoneSemantics = optionalBool(g, 9).value_or(false);
        out.readOnly = optionalBool(g, 10).value_or(false);
        out.recoilTimeState = optionalBool(g, 11).value_or(false);
        out.mapIdentity = optionalBool(g, 12).value_or(false);
        out.boneWorldPose = optionalBool(g, 13).value_or(false);
        out.weaponAccuracyEvaluation = optionalBool(g, 14).value_or(false);
        require(out.full && out.readOnly, "CS2 service capability missing");
        require(!out.independentGroupTimestamps, "CS2 independent group timestamps unsupported");
    }
    else if (out.body == 12)
    {
        out.request = number(g, 1, 1);
        const auto captureRate = one(g, 2, 1);
        require(captureRate, "CS2 subscribed capture rate missing");
        out.captureHz = real(*captureRate);
        auto rate = one(g, 3, 1);
        require(rate, "CS2 publish rate missing");
        out.publishHz = real(*rate);
    }
    else if (out.body == 13)
        out.snapshot = decodeSnapshot(g, value->bytes);
    else if (out.body == 14)
    {
        const auto availability = number(g, 1);
        out.availability = availability >= 1 && availability <= 4 ? static_cast<unsigned>(availability) : 3;
        out.statusEpoch = optional64(g, 2);
        out.error = reasonText(optionalUnsigned(g, 5).value_or(0));
        if (availability != out.availability && out.error.empty())
            out.error = "CS2 Reader 未知可用性 " + std::to_string(availability);
    }
    else if (out.body == 16)
    {
        out.request = number(g, 1, 1);
        auto [d, q] = decodeClock(bytes(g, 2));
        out.clockDomain = d;
        out.frequency = q;
        out.received = optional64(g, 3);
        auto sent = optional64(g, 4);
        require(!sent || !out.sent || sent == out.sent, "CS2 clock stamps disagree");
        out.sent = sent;
    }
    else if (out.body == 17)
    {
        out.request = number(g, 1, 1);
        const auto reason = optionalUnsigned(g, 4).value_or(0);
        require(out.request && reason, "CS2 Reader 拒绝回执缺少请求编号或原因");
        out.error = reasonText(reason);
    }
    return out;
}
void validateSnapshotCapabilities(const Envelope &hello, const games::cs2::Snapshot &frame)
{
    require(hello.body == 10 && hello.full && hello.readOnly && !hello.independentGroupTimestamps,
            "CS2 snapshot without supported Hello capabilities");
    for (const auto &player : frame.players)
        if (player.body)
        {
            require(hello.verifiedBoneSemantics ||
                        (!player.body->skeletonStatus && !player.body->skeletonModel && !player.body->skeletonProfile &&
                         player.body->bones.empty() && player.body->boneLinks.empty()),
                    "CS2 skeleton group contradicts disabled Hello capability");
            require(hello.recoilTimeState || !player.body->aimPunch,
                    "CS2 aim punch contradicts disabled Hello capability");
            bool hasWorldPose = player.body->nativeHitboxSetAvailable.has_value();
            for (const auto &bone : player.body->bones)
                hasWorldPose = hasWorldPose || bone.worldRotation.has_value() || bone.worldUniformScale.has_value();
            require(hello.boneWorldPose || !hasWorldPose, "CS2 bone world pose contradicts disabled Hello capability");
        }
    require(hello.recoilTimeState || !frame.local.viewPunch, "CS2 camera punch contradicts disabled Hello capability");
    require(hello.mapIdentity || !frame.map, "CS2 map identity contradicts disabled Hello capability");
}
std::string record(std::string b)
{
    require(!b.empty() && b.size() <= MaximumRecord, "CS2 record bound");
    std::string p(4, '\0');
    auto n = static_cast<unsigned>(b.size());
    for (unsigned i = 0; i < 4; ++i)
        p[i] = static_cast<char>(n >> (24 - i * 8));
    return p + b;
}
std::string subscribe(std::uint64_t id, double hz)
{
    require(std::isfinite(hz) && hz >= 1, "CS2 subscription rate invalid");
    std::string b;
    putNumber(b, 1, id, 1);
    putNumber(b, 2, Version);
    putBytes(b, 3, schemaBytes());
    putNumber(b, 4, std::bit_cast<std::uint64_t>(hz), 1);
    return envelope(11, std::move(b));
}
std::string clockQuery(std::uint64_t id)
{
    std::string b;
    putNumber(b, 1, id, 1);
    return envelope(15, std::move(b));
}
} // namespace mousebox::sources::cs2_reader
