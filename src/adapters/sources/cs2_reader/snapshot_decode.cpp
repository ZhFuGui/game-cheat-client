#include "snapshot_decode.hpp"
#include "games/cs2/model/snapshot.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>

namespace mousebox::sources::cs2_reader::detail
{
using namespace games::cs2;
namespace
{
std::optional<std::uint32_t> handle(const Fields &f, unsigned id)
{
    auto p = one(f, id, 2);
    if (!p)
        return {};
    auto inner = fields(p->bytes);
    return static_cast<std::uint32_t>(number(inner, 1, 5));
}
Vec3 vector(std::string_view b)
{
    auto f = fields(b);
    return {optionalFloat(f, 1).value_or(0), optionalFloat(f, 2).value_or(0), optionalFloat(f, 3).value_or(0)};
}
std::optional<Vec3> optionalVector(const Fields &f, unsigned id)
{
    auto p = one(f, id, 2);
    return p ? std::optional(vector(p->bytes)) : std::nullopt;
}
template <typename T, typename Decode>
std::optional<T> optionalMessage(const Fields &f, unsigned id, Decode decodeValue)
{
    const auto p = one(f, id, 2);
    return p ? std::optional<T>(decodeValue(fields(p->bytes))) : std::nullopt;
}
GameTime gameTime(const Fields &f)
{
    return {optionalInt(f, 1), optionalFloat(f, 2)};
}
RecoilTimes recoilTimes(const Fields &f)
{
    RecoilTimes out;
    if (const auto group = one(f, 1, 5))
        out.worldGroup = static_cast<std::uint32_t>(group->n);
    const auto policy = number(f, 2);
    require(policy <= 2, "CS2 invalid visual recoil time policy");
    out.visualPolicy = static_cast<VisualRecoilPolicy>(policy);
    out.simulation = optionalMessage<GameTime>(f, 3, gameTime);
    out.visualPredictable = optionalMessage<GameTime>(f, 4, gameTime);
    out.visualUnpredictable = optionalMessage<GameTime>(f, 5, gameTime);
    out.secondsPerTick = optionalFloat(f, 6);
    out.nativeContext = optionalInt(f, 7);
    require(!out.secondsPerTick || *out.secondsPerTick > 0, "CS2 invalid game tick interval");
    return out;
}
AimPunchState aimPunch(const Fields &f)
{
    AimPunchState out;
    out.predictable = optionalMessage<PredictablePunch>(f, 1, [](const Fields &g) {
        return PredictablePunch{optionalMessage<GameTime>(g, 1, gameTime), optionalVector(g, 2), optionalVector(g, 3)};
    });
    out.unpredictable = optionalMessage<UnpredictablePunch>(
        f, 2, [](const Fields &g) { return UnpredictablePunch{optionalInt(g, 1), optionalVector(g, 2)}; });
    out.times = optionalMessage<RecoilTimes>(f, 3, recoilTimes);
    return out;
}
CameraViewPunch cameraViewPunch(const Fields &f)
{
    return {optionalVector(f, 1), optionalMessage<GameTime>(f, 2, gameTime), optionalFloat(f, 3), optionalFloat(f, 4)};
}
MouseMapping mouseMapping(const Fields &f)
{
    const auto source = number(f, 1);
    require(source <= 2, "CS2 invalid native input source");
    MouseMapping out;
    out.source = static_cast<NativeInputSource>(source);
    out.sensitivity = optionalFloat(f, 2);
    out.verticalScale = optionalFloat(f, 3);
    out.yaw = optionalFloat(f, 4);
    out.pitch = optionalFloat(f, 5);
    out.zoomRatio = optionalFloat(f, 6);
    out.invertedY = optionalBool(f, 7);
    out.effectivePitch = optionalFloat(f, 8);
    out.pawnFovAdjust = optionalFloat(f, 9);
    out.pawnMouseSensitivity = optionalFloat(f, 10);
    out.yawDegreesPerDelta = optionalFloat(f, 11);
    out.pitchDegreesPerDelta = optionalFloat(f, 12);
    require(out.source == NativeInputSource::Mouse || (!out.yawDegreesPerDelta && !out.pitchDegreesPerDelta),
            "CS2 mouse derivatives contradict input source");
    return out;
}
EyeAngleBasis eyeBasis(const Fields &f)
{
    const auto source = number(f, 2);
    require(source <= 7, "CS2 invalid eye angle basis source");
    const auto resolved = optionalUnsigned(f, 5);
    require(!resolved || *resolved <= 7, "CS2 invalid resolved eye angle source");
    EyeAngleBasis out{optionalVector(f, 1), static_cast<EyeAngleSource>(source), optionalInt(f, 3),
                      optionalFloat(f, 4)};
    if (resolved)
        out.resolvedSource = static_cast<EyeAngleSource>(*resolved);
    out.sourceEntity = handle(f, 6);
    out.delegationDepth = optionalUnsigned(f, 7);
    return out;
}
Entity entity(std::string_view b)
{
    auto f = fields(b);
    Entity e;
    auto id = handle(f, 1);
    require(id.has_value(), "CS2 entity identity missing");
    e.id = *id;
    e.name = text(bytes(f, 2));
    require(e.name.size() <= 4096, "CS2 class name bound");
    e.position = optionalVector(f, 3);
    e.absoluteRotation = optionalVector(f, 4);
    e.absoluteScale = optionalFloat(f, 5);
    e.velocity = optionalVector(f, 7);
    e.team = optionalUnsigned(f, 8);
    e.health = optionalInt(f, 9);
    e.maxHealth = optionalInt(f, 10);
    e.lifeState = optionalUnsigned(f, 11);
    e.dormant = optionalBool(f, 6);
    return e;
}
void skeleton(Body &out, std::string_view b)
{
    const auto sk = fields(b);
    const auto status = number(sk, 2);
    require(status <= static_cast<unsigned>(SkeletonStatus::VerifiedComplete), "CS2 invalid skeleton status");
    out.skeletonStatus = static_cast<SkeletonStatus>(status);
    out.nativeHitboxSetAvailable = optionalBool(sk, 6);
    if (const auto model = one(sk, 3, 2))
    {
        out.skeletonModel = text(model->bytes);
        require(!out.skeletonModel->empty(), "CS2 empty skeleton model");
    }
    if (const auto profile = one(sk, 4, 2))
    {
        out.skeletonProfile = text(profile->bytes);
        require(!out.skeletonProfile->empty(), "CS2 empty skeleton profile");
    }
    std::set<unsigned> indices, roles;
    for (const auto &point : many(sk, 1))
    {
        require(point.wire == 2 && out.bones.size() < 21, "CS2 skeleton bound/type");
        const auto bone = fields(point.bytes);
        const auto index = optionalUnsigned(bone, 1).value_or(0);
        require(index && index <= UINT16_MAX && indices.insert(index).second, "CS2 invalid/duplicate bone index");
        require(one(bone, 2, 2), "CS2 bone position missing");
        Bone value{index, vector(bytes(bone, 2)), {}};
        const auto rotation = one(bone, 4, 2);
        const auto scale = optionalFloat(bone, 5);
        require(bool(rotation) == scale.has_value(), "CS2 incomplete bone world pose group");
        if (rotation)
        {
            const auto values = fields(rotation->bytes);
            std::array<double, 4> q;
            double norm = 0;
            for (unsigned i = 0; i < 4; ++i)
            {
                q[i] = optionalFloat(values, i + 1).value_or(0);
                norm += q[i] * q[i];
            }
            require(std::abs(norm - 1) <= .001, "CS2 invalid bone world pose");
            value.worldRotation = q;
            value.worldUniformScale = scale;
        }
        if (const auto role = optionalUnsigned(bone, 3))
        {
            require(*role <= static_cast<unsigned>(BoneRole::RightAnkle), "CS2 invalid bone role");
            require(!*role || roles.insert(*role).second, "CS2 duplicate bone role");
            value.role = static_cast<BoneRole>(*role);
        }
        out.bones.push_back(std::move(value));
    }
    std::map<unsigned, unsigned> parents;
    for (const auto &edge : many(sk, 5))
    {
        require(edge.wire == 2 && out.boneLinks.size() < 20, "CS2 skeleton link bound/type");
        const auto link = fields(edge.bytes);
        const auto parent = optionalUnsigned(link, 1).value_or(0);
        const auto child = optionalUnsigned(link, 2).value_or(0);
        require(parent != child && indices.contains(parent) && indices.contains(child), "CS2 skeleton link endpoint");
        require(parents.emplace(child, parent).second, "CS2 duplicate skeleton link/parent");
        out.boneLinks.push_back({parent, child});
    }
    for (const auto &[child, parent] : parents)
    {
        std::set<unsigned> ancestors{child};
        auto current = parent;
        for (;;)
        {
            require(ancestors.insert(current).second, "CS2 cyclic skeleton links");
            const auto next = parents.find(current);
            if (next == parents.end())
                break;
            current = next->second;
        }
    }
    const bool verified =
        out.skeletonStatus == SkeletonStatus::VerifiedPartial || out.skeletonStatus == SkeletonStatus::VerifiedComplete;
    if (verified)
    {
        require(out.skeletonModel && out.skeletonProfile, "CS2 verified skeleton identity missing");
        if (out.skeletonStatus == SkeletonStatus::VerifiedComplete)
            require(out.bones.size() == 21 && out.boneLinks.size() == 20, "CS2 incomplete verified skeleton");
    }
    else
    {
        require(out.bones.empty() && out.boneLinks.empty(), "CS2 unavailable skeleton contains geometry");
        require(out.skeletonStatus != SkeletonStatus::UnknownModel || !out.skeletonProfile,
                "CS2 unknown model cannot claim a verified profile");
    }
}
Body body(std::string_view b)
{
    auto f = fields(b);
    Body out;
    require(one(f, 1, 2), "CS2 body entity missing");
    out.entity = entity(bytes(f, 1));
    out.armor = optionalInt(f, 2);
    out.shotsFired = optionalInt(f, 3);
    out.scoped = optionalBool(f, 4);
    out.helmet = optionalBool(f, 6);
    out.networkEyeAngles = optionalVector(f, 12);
    out.inputEyeAngles = optionalVector(f, 20);
    out.eyeBasis = optionalMessage<EyeAngleBasis>(f, 21, eyeBasis);
    // An unusable semantic basis withholds only this group, not the body,
    // its independent punch state, skeleton, or the complete snapshot.
    if (out.eyeBasis && !validEyeBasis(*out.eyeBasis))
        out.eyeBasis.reset();
    out.aimPunch = optionalMessage<AimPunchState>(f, 13, aimPunch);
    require(!f.contains(14), "CS2 retired aim punch field");
    out.viewOffset = optionalVector(f, 15);
    out.activeWeapon = handle(f, 17);
    if (auto p = one(f, 18, 2))
        skeleton(out, p->bytes);
    return out;
}
Player player(std::string_view b)
{
    auto f = fields(b);
    Player out;
    auto id = handle(f, 1);
    require(id.has_value(), "CS2 controller missing");
    out.controller = *id;
    if (auto n = one(f, 2, 2))
    {
        require(n->bytes.size() <= 4096, "CS2 player name bound");
        out.name = text(n->bytes);
    }
    out.team = optionalUnsigned(f, 10);
    out.local = optionalBool(f, 11);
    out.alive = optionalBool(f, 12);
    out.steamId = optional64(f, 4);
    if (auto p = one(f, 17, 2))
        out.body = body(p->bytes);
    return out;
}
Local local(std::string_view b)
{
    auto f = fields(b);
    Local out;
    out.controller = handle(f, 1);
    out.pawn = handle(f, 2);
    out.observerTarget = handle(f, 3);
    out.angles = optionalVector(f, 6);
    out.viewPunch = optionalMessage<CameraViewPunch>(f, 7, cameraViewPunch);
    out.eye = optionalVector(f, 8);
    out.fovOverride = optionalUnsigned(f, 9);
    out.observerMode = optionalUnsigned(f, 10);
    require(!f.contains(11), "CS2 retired standalone sensitivity field");
    out.mouseMapping = optionalMessage<MouseMapping>(f, 12, mouseMapping);
    out.crosshairRecoil = optionalBool(f, 13);
    out.cameraRecoilPawn = handle(f, 14);
    const auto path = number(f, 15);
    require(path <= 2, "CS2 invalid camera recoil path");
    out.cameraRecoilPath = static_cast<CameraRecoilPath>(path);
    require(path || (!out.viewPunch && !out.cameraRecoilPawn), "CS2 unknown camera path has feedback source");
    if (auto p = one(f, 5, 2))
    {
        auto m = fields(p->bytes);
        std::vector<double> values;
        for (const auto &v : many(m, 1))
        {
            require(values.size() < 16, "CS2 matrix element bound");
            if (v.wire == 5)
                values.push_back(real(v));
            else
            {
                require(v.wire == 2 && v.bytes.size() % 4 == 0, "CS2 matrix packed type");
                require(v.bytes.size() / 4 <= 16 - values.size(), "CS2 matrix element bound");
                for (std::size_t j = 0; j < v.bytes.size(); j += 4)
                {
                    std::uint32_t u = 0;
                    for (unsigned k = 0; k < 4; ++k)
                        u |= std::uint32_t(static_cast<unsigned char>(v.bytes[j + k])) << (k * 8);
                    values.push_back(real({5, u, {}}));
                }
            }
        }
        require(values.size() == 16, "CS2 matrix must have 16 elements");
        std::array<double, 16> matrix{};
        std::copy(values.begin(), values.end(), matrix.begin());
        out.matrix = matrix;
    }
    return out;
}
MapIdentity mapIdentity(const Fields &f)
{
    const auto status = number(f, 1);
    require(status <= 5, "CS2 invalid map status");
    MapIdentity out;
    out.status = static_cast<MapStatus>(status);
    out.generation = number(f, 2, 1);
    const auto name = one(f, 3, 2), path = one(f, 4, 2);
    if (out.status == MapStatus::Available)
    {
        require(out.generation && name && path, "CS2 available map identity is incomplete");
        out.name = text(name->bytes);
        out.resourcePath = text(path->bytes);
        require(!out.name.empty() && !out.resourcePath.empty() && out.name.find('\0') == std::string::npos &&
                    out.resourcePath.find('\0') == std::string::npos,
                "CS2 map identity text is empty or contains NUL");
    }
    else
        require(!name && !path, "CS2 unavailable map must not carry a stale name or resource path");
    return out;
}
AccuracyScalar accuracyScalar(const Fields &f, unsigned id)
{
    AccuracyScalar out;
    try
    {
        if (const auto p = one(f, id, 2))
        {
            const auto scalar = fields(p->bytes);
            out.status = optionalUnsigned(scalar, 2).value_or(0);
            out.value = optionalFloat(scalar, 1);
            out.malformed = out.status > 14 || ((out.status == 1) != out.value.has_value());
            if (out.malformed)
                out.value.reset();
        }
    }
    catch (const std::runtime_error &)
    {
        out = {};
        out.malformed = true;
    }
    return out;
}
WeaponAccuracy accuracy(const Fields &f)
{
    WeaponAccuracy out;
    out.inaccuracy = accuracyScalar(f, 3);
    out.spread = accuracyScalar(f, 4);
    try
    {
        out.modelId = text(bytes(f, 1));
        require(out.modelId.size() <= 128, "CS2 accuracy model bound");
        out.started = optional64(f, 13);
        out.readFinished = optional64(f, 14);
    }
    catch (const std::runtime_error &)
    {
        out.malformed = true;
    }
    try
    {
        out.owner = handle(f, 2);
        out.branch = optionalUnsigned(f, 5).value_or(0);
        out.movement = optionalFloat(f, 6);
        out.airborne = optionalFloat(f, 7);
        out.turning = optionalFloat(f, 8);
        out.lateral = optionalFloat(f, 9);
        out.penalty = optionalFloat(f, 10);
        out.mode = optionalInt(f, 11);
        out.stateTime = optionalFloat(f, 12);
    }
    catch (const std::runtime_error &)
    {
        out.inaccuracy.malformed = true;
    }
    return out;
}
} // namespace

std::shared_ptr<const Snapshot> decodeSnapshot(const Fields &f, std::string_view wire)
{
    auto out = std::make_shared<Snapshot>();
    out->wire = std::string(wire);
    auto id = fields(bytes(f, 1));
    auto instance = bytes(id, 1);
    require(instance.size() == 16, "CS2 snapshot instance missing");
    out->instance = hex(instance);
    out->epoch = number(id, 2, 1);
    out->sequence = number(id, 3, 1);
    require(out->epoch && out->sequence, "CS2 capture identity invalid");
    out->processId = static_cast<std::uint32_t>(number(id, 4));
    out->build = static_cast<std::uint32_t>(number(id, 5));
    auto [domain, freq] = decodeClock(bytes(f, 2));
    out->clockDomain = std::move(domain);
    out->frequency = freq;
    auto interval = fields(bytes(f, 3));
    out->started = optional64(interval, 1);
    out->readFinished = optional64(interval, 2);
    out->finished = optional64(interval, 3);
    if (out->started && out->readFinished)
        require(*out->started <= *out->readFinished, "CS2 read interval reversed");
    if (out->readFinished && out->finished)
        require(*out->readFinished <= *out->finished, "CS2 finished interval reversed");
    if (out->started && out->finished)
        require(*out->started <= *out->finished, "CS2 capture interval reversed");
    auto coverage = fields(bytes(f, 4));
    std::uint64_t sum = 0;
    for (unsigned i = 2; i <= 7; ++i)
    {
        auto n = number(coverage, i, 1);
        require(n <= UINT64_MAX - sum, "CS2 coverage overflow");
        sum += n;
    }
    require(number(coverage, 1, 1) == sum, "CS2 coverage sum mismatch");
    out->directoryVerified = number(coverage, 10) != 0;
    std::set<std::uint32_t> controllers, weaponIds;
    for (const auto &p : many(f, 5))
    {
        require(p.wire == 2 && out->players.size() < 4096, "CS2 player bound/type");
        auto value = player(p.bytes);
        require(controllers.insert(value.controller).second, "CS2 duplicate controller");
        out->players.push_back(std::move(value));
    }
    for (const auto &p : many(f, 6))
    {
        require(p.wire == 2 && out->weapons.size() < 32768, "CS2 weapon bound/type");
        auto w = fields(p.bytes);
        Weapon value;
        value.entity = entity(bytes(w, 1));
        value.definition = optionalUnsigned(w, 2);
        value.clip = optionalInt(w, 4);
        value.reserve = optionalInt(w, 6);
        value.reloading = optionalBool(w, 12);
        value.accuracyPenalty = optionalFloat(w, 10);
        value.recoilIndex = optionalFloat(w, 11);
        if (const auto group = one(w, 15, 2))
            value.accuracy = accuracy(fields(group->bytes));
        require(weaponIds.insert(value.entity.id).second, "CS2 duplicate weapon");
        out->weapons.push_back(std::move(value));
    }
    out->bombs = many(f, 7).size();
    out->projectiles = many(f, 8).size();
    out->worldObjects = many(f, 9).size();
    if (auto p = one(f, 10, 2))
        out->local = local(p->bytes);
    if (auto world = one(f, 11, 2))
        out->map = optionalMessage<MapIdentity>(fields(world->bytes), 13, mapIdentity);
    return out;
}
} // namespace mousebox::sources::cs2_reader::detail
