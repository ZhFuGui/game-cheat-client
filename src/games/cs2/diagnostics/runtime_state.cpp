#include "runtime_state.hpp"
#include "recoil_state.hpp"
#include "map_state.hpp"
#include "viewpoint_state.hpp"
#include "accuracy_state.hpp"
#include "adapters/sources/cs2_reader/client.hpp"
#include "domain/connection_policy.hpp"
#include "application/diagnostics/reader_chain.hpp"
namespace mousebox::games::cs2
{
using application::Json;
namespace
{
Json nullable(std::optional<double> v)
{
    return v ? Json(*v) : Json{};
}
} // namespace
Json describeSnapshot(const sources::cs2_reader::Observation &o, double scale, bool diagnostics)
{
    auto f = o.current ? o.frame : nullptr;
    std::string phase = !o.requested   ? "closed"
                        : o.connecting ? "connecting"
                        : !o.connected ? "error"
                        : !o.current   ? "degraded"
                                       : "ready";
    std::string message = !o.requested       ? "CS2 Reader 未连接"
                          : !o.error.empty() ? o.error
                          : o.current        ? "CS2 Reader 已连接"
                                             : "CS2 Reader 等待完整快照";
    if (f && !f->local.matrix)
    {
        phase = "degraded";
        message = "CS2 Reader 等待视图矩阵";
    }
    if (!o.subscriptionError.empty() && o.connected)
    {
        phase = "degraded";
        message = "CS2订阅改频被拒绝；保留已确认频率：" + o.subscriptionError;
    }
    auto metrics = o.metrics;
    Json channels = Json::array();
    if (o.requested)
        channels.push_back({{"category", 1},
                            {"connection", std::to_string(metrics.connection)},
                            {"name", "CS2"},
                            {"status", !metrics.frames    ? "waiting-data"
                                       : !metrics.clock   ? "waiting-clock"
                                       : !metrics.samples ? "waiting-draw"
                                                          : "ready"},
                            {"averageMs", nullable(metrics.averageMs)},
                            {"uncertaintyMs", nullable(metrics.uncertaintyMs)},
                            {"samples", metrics.samples},
                            {"receivedFramesPerSecond", metrics.frames},
                            {"chain", application::describeReaderChain(metrics.chain)}});
    const Player *self = nullptr;
    const Weapon *weapon = nullptr;
    if (f && f->local.controller)
        for (const auto &p : f->players)
            if (p.controller == *f->local.controller)
            {
                self = &p;
                break;
            }
    if (f && self && self->body && self->body->activeWeapon)
        for (const auto &w : f->weapons)
            if (w.entity.id == *self->body->activeWeapon)
            {
                weapon = &w;
                break;
            }
    auto vec = [](const std::optional<Vec3> &v) -> Json { return v ? Json::array({v->x, v->y, v->z}) : Json{}; };
    Json state = {
        {"connections", {{"reader", {{"phase", phase}, {"message", message}, {"retryable", true}}}}},
        {"automaticConnectionIntervalMs", AutomaticConnectionIntervalMs},
        {"readerRunning", o.requested},
        {"cs2RequestedHz", o.requestedHz},
        {"cs2SubscribedHz", nullable(o.subscribedHz)},
        {"cs2CaptureHz", nullable(o.captureHz)},
        {"cs2SubscriptionPending", o.subscriptionPending},
        {"cs2SubscriptionError", o.subscriptionError},
        {"readerConnected", o.connected},
        {"cs2Map", f ? describeMap(f->map) : Json{}},
        {"canAim", false},
        {"canCalibrate", false},
        {"aimArmed", false},
        {"aimAutomatic", false},
        {"aimReference", "pending"},
        {"characters", f ? Json(f->players.size()) : Json{}},
        {"target", nullptr},
        {"gap", nullptr},
        {"fov", nullptr},
        {"angle", nullptr},
        {"targetDistance", nullptr},
        {"control", "CS2 原生映射、后坐和条件眼角基准已只读接入；设备校准与CS2跟随闭环未验收，瞄准输出未开放。"},
        {"localBodyKnown", self && self->body.has_value()},
        {"hasCurrentItem", self && self->body ? Json(self->body->activeWeapon.has_value()) : Json{}},
        {"currentWeaponClass", weapon ? Json(weapon->entity.name) : Json{}},
        {"spectators", nullptr},
        {"espSequences", f ? "CS2 #" + std::to_string(f->sequence) : "CS2 —"},
        {"espFov", "Reader world-to-clip matrix"},
        {"espUnits", scale > 0 ? "手动世界单位比例；距离从本地角色眼部计算" : "Source 2 原生单位；米制比例尚未设置"},
        {"cs2ProcessId", f ? Json(f->processId) : Json{}},
        {"cs2Build", f ? Json(f->build) : Json{}},
        {"cs2FovOverride", f && f->local.fovOverride ? Json(*f->local.fovOverride) : Json{}},
        {"cs2Sensitivity", f && f->local.mouseMapping ? nullable(f->local.mouseMapping->sensitivity) : Json{}},
        {"cs2ViewAngles", f ? vec(f->local.angles) : Json{}},
        {"cs2Viewpoint", f ? describeViewpoint(*f) : Json{}},
        {"cs2Accuracy", f ? describeAccuracy(*f, o.weaponAccuracyEvaluation) : Json{}},
        {"cs2ViewPunch", f && f->local.viewPunch ? vec(f->local.viewPunch->baseAngle) : Json{}},
        {"cs2PredictablePunch", self && self->body && self->body->aimPunch && self->body->aimPunch->predictable
                                    ? vec(self->body->aimPunch->predictable->baseAngle)
                                    : Json{}},
        {"cs2UnpredictablePunch", self && self->body && self->body->aimPunch && self->body->aimPunch->unpredictable
                                      ? vec(self->body->aimPunch->unpredictable->baseAngle)
                                      : Json{}},
        {"cs2Recoil", f ? describeRecoil(*f) : Json{}},
        {"cs2RecoilIndex", weapon ? nullable(weapon->recoilIndex) : Json{}},
        {"cs2AccuracyPenalty", weapon ? nullable(weapon->accuracyPenalty) : Json{}},
        {"cs2Health", self && self->body && self->body->entity.health ? Json(*self->body->entity.health) : Json{}},
        {"cs2ShotsFired", self && self->body && self->body->shotsFired ? Json(*self->body->shotsFired) : Json{}},
        {"cs2Weapons", f ? Json(f->weapons.size()) : Json{}},
        {"cs2Bombs", f ? Json(f->bombs) : Json{}},
        {"cs2Projectiles", f ? Json(f->projectiles) : Json{}},
        {"readerMetrics",
         {{"windowMs", 1000},
          {"uploadBytesPerSecond", metrics.upload},
          {"downloadBytesPerSecond", metrics.download},
          {"gpuCompletionAvailable", false},
          {"channels", std::move(channels)}}}};
    Json algorithm = Json::object(), sections = Json::array();
    if (diagnostics)
    {
        Json facts = Json::object();
        const auto add = [&](const char *id, const char *title, const char *current, const char *principle,
                             const char *status) {
            facts[id] = {{"id", id},
                         {"title", title},
                         {"current", current},
                         {"source", "cs2.reader.v2 / 原生独立 CS2 适配"},
                         {"principle", principle},
                         {"status", status},
                         {"phase", std::string_view(status) == "implemented"   ? "wired"
                                   : std::string_view(status) == "unavailable" ? "missing"
                                                                               : "observed"}};
        };
        add("cs2-transport", "只读数据源", "完整快照",
            "版本和摘要握手；实例、事件序号、会话及完整句柄隔离，失效或断流清除旧帧。", "implemented");
        add("cs2-time", "时间来源", "单一完整批次",
            "Local、人物、武器共享完整读取区间；时钟往返映射，心跳不刷新采样时间。", "implemented");
        add("cs2-projection", "ESP 投影", "Reader 世界到裁剪矩阵",
            "行主序矩阵左乘世界坐标；独立原生 DirectX 投影，不使用 PUBG FOV。", "implemented");
        add("cs2-skeleton", "骨架", "按模型核验的主干骨架",
            "只显示本批已核验模型的骨点与父子边；语义来自 Bone.role，不按索引猜测，缺点不沿用。HEAD "
            "是骨原点，不是头顶或命中盒。",
            "implemented");
        add("cs2-units", "世界单位", "Source 2 原生单位",
            "米制比例默认未知；用户明确设置后才显示角色眼部到实体的米制距离。", "limited");
        add("cs2-input", "鼠标输入", "原生映射与独立设备校准",
            "Reader "
            "的度/"
            "原生增量已含符号、灵敏度、纵轴和开镜倍率，不重复相乘；零值不能作除数，Analog不冒充鼠标。设备换算由独立实测"
            "记录提供。",
            "limited");
        add("cs2-punch", "后坐求值", "按本批原生游戏时间纯计算",
            "冻结求值器完整衰减/速度/插值后乘二。网络眼角+P+U仅为派生对照，不冒充native "
            "EyeAngles或武器方向；相机为视觉P/"
            "U的0.45倍加独立view-punch；本地跟随后坐准星只用P。观战相机按完整来源handle关联，不重复叠加到矩阵。",
            "implemented");
        add("cs2-recoil-time", "游戏时间与执行延迟", "区分采样求值与模型预测",
            "原生tick为1/64秒，P/"
            "U视觉时刻可不同。数据看板按本批时刻求值；控制器在明确无新冲量、游戏时间等速前进的假设下外推。QPC仅估计批次"
            "年龄，不冒充原生指令时间。",
            "implemented");
        add("cs2-eye-basis", "眼角基准", "Reader 已求值的成功分支",
            "来源1–7已由Reader完成网络、预测、缓存、历史、父变换或委托求值，不重复变换。普通本地无委托/"
            "父变换的source2输入"
            "角已证明不依赖视觉选择器；其他分支只与所属人物AimPunch原生上下文一致才组合。委托元数据仅解释来源，不替换后"
            "坐所"
            "有者或要求来源实体先出现在人物列表；缺失不回退。",
            "limited");
        add("cs2-validation", "运行验证", "Reader人工记录已核对，设备闭环待验收",
            "单发、连发、停火、换枪、变倍、受击和第一人称观战记录已核对；其他模式、物理设备校准及CS2跟随闭环未验收。sho"
            "ts_fired、recoil_index和accuracy_penalty不是is_firing/can_fire。",
            "limited");
        add("cs2-ballistics", "弹道", "未启用 PUBG 模型", "不套用 PUBG 的弹速、下坠、归零和枪口参数。", "unavailable");
        algorithm = {
            {"summary", "CS2：只读数据 / 矩阵 ESP；瞄准输出待核验"}, {"revision", "cs2-reader-v2"}, {"facts", facts}};
        sections.push_back({{"title", "CS2 数据来源与能力"},
                            {"ids", Json::array({"cs2-transport", "cs2-time", "cs2-projection", "cs2-skeleton",
                                                 "cs2-units", "cs2-input", "cs2-punch", "cs2-recoil-time",
                                                 "cs2-eye-basis", "cs2-validation", "cs2-ballistics"})}});
    }
    return {
        {"state", std::move(state)}, {"algorithm", std::move(algorithm)}, {"algorithmSections", std::move(sections)}};
}
} // namespace mousebox::games::cs2
