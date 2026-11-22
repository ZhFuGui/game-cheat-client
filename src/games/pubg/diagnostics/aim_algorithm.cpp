#include "aim_algorithm.hpp"
#include "games/pubg/runtime/status_text.hpp"
#include "application/diagnostics/text.hpp"
#include "games/pubg/pipeline/latency_prediction.hpp"
#include "reader_telemetry.hpp"
#include "timing_metadata.hpp"

#include <algorithm>
#include <cmath>
namespace mousebox::application
{
namespace
{
Text tr(const char *s)
{
    return Text(s);
}
Text num(double v, int digits = 2)
{
    return std::isfinite(v) ? Text::number(v, 'f', digits) : Text("—");
}
Text vector(control::Vec3 v)
{
    return Text("(%1, %2, %3)").arg(num(v.x), num(v.y), num(v.z));
}
Text predictionReason(control::PredictionReason reason)
{
    using R = control::PredictionReason;
    switch (reason)
    {
    case R::Applied:
        return tr("补偿可用");
    case R::TotalWorldVelocityMissing:
        return tr("缺少当前速度");
    case R::GeometryMissing:
        return tr("缺少部位坐标");
    case R::InvalidVelocity:
        return tr("速度无效或身份不符");
    case R::SourceTimingInvalid:
        return tr("采样区间无效");
    case R::VelocitySampleSkew:
        return tr("骨点与速度采样差过大");
    case R::ClockUnavailable:
        return tr("缺少时钟映射");
    case R::ClockStale:
        return tr("时钟探测过期");
    case R::ClockUncertain:
        return tr("时钟误差过大");
    case R::AgeOutOfRange:
        return tr("数据年龄超出补偿范围");
    }
    return tr("等待有效数据");
}
} // namespace
AimAlgorithmReport describeAimAlgorithm(const Snapshot &s, const control::LiveDiagnostic &d, std::uint64_t now)
{
    using namespace control;
    Json facts = Json::object();
    const auto add = [&](const char *id, const char *title, const char *phase, const Text &current, const Text &source,
                         const Text &principle, bool available = false) {
        const auto status = Text(phase) == "wired"         ? tr("已接入")
                            : Text(phase) == "conditional" ? tr("条件使用")
                            : Text(phase) == "agreed"      ? tr("用户约定 · 未接入")
                            : Text(phase) == "missing"     ? tr("待补齐 · 未接入")
                            : Text(phase) == "observed"    ? tr("只读观测 · 不新增补偿")
                                                           : tr("未参与");
        facts[id] =
            Json{{"id", id},           {"title", tr(title)}, {"phase", phase},         {"status", status},
                 {"current", current}, {"source", source},   {"principle", principle}, {"available", available}};
    };
    const auto p = speedPolicy(s.speed, s.device.motionLimits);
    const auto target = d.target;
    const auto prediction = target ? predictPosition(*target, now) : PositionPrediction{};
    const auto calibration = validCalibration(s.calibration);
    const auto waiting = tr("等待有效数据");
    const auto &assumptions = CurrentBallisticsAssumptions;
    const auto gravity = s.motion.gravityMetersPerSecondSquared;
    const auto gravityText = Text::number(gravity, 'g', 15);
    const bool integrated = assumptions.liveBallisticsIntegrated;
    const auto &b = d.ballistics;
    const bool ready = b.block == BallisticBlock::Ready;
    const bool snapshotViewOnly = s.view && s.view->nativeAimBasis && s.view->nativeAimBasis->branch == 2;
    const auto feedback =
        s.controlActive && !(snapshotViewOnly && s.motionFeedback && s.motionFeedback->nativeReference)
            ? s.motionFeedback
            : std::nullopt;
    const bool nativeActive = feedback && feedback->nativeReference;
    const auto pending = tr("等待当前目标与弹道输入");
    const Text ballisticStatus = b.block == BallisticBlock::Disabled ? pending : Text(ballisticBlockText(b.block));
    const auto catalog = s.motion.ballisticCatalog;
    const CompiledBallisticProfile *profile = nullptr;
    if (catalog && s.view && s.view->activeWeapon)
    {
        const auto &w = *s.view->activeWeapon;
        profile = catalog->find(w.actorClass, w.itemId).entry;
    }
    add("shot_direction", "参考发射方向", integrated ? "conditional" : "agreed",
        tr("以相机为弹道起点；原生参考使用发射方向，相机参考使用等效归零反解的视线。"),
        tr("用户于2026-10-16指定相机起点、同向近似和等效归零；不是实测游戏规则。"),
        tr("先求相机起点下的D_launch。相机模式扣所选等效归零得到D_aim；合格原生basis已含原生归零，直接与D_"
           "launch比较。枪口位置不作弹道起点，也不声称相机是游戏真实子弹生成点。"),
        ready);
    add("flight_rule", "弹速衰减与飞行时间", integrated ? "conditional" : "agreed",
        ready ? tr("飞行 %1 ms；曲线步进距离 %2 m；%3。")
                    .arg(num(b.solution.flightSeconds * 1000, 2), num(b.solution.travelMeters, 2),
                         b.solution.usedCurve ? tr("公开曲线求积") : tr("无足够曲线点，使用初速"))
              : ballisticStatus,
        Text("pubg.gs/ballistics/ · 1486rtku6u17m.js · Build25901960 · 2026-10-14"),
        tr("速度分段线性插值，ΔT≈Δs(1/v起+1/v终)/2；末点后恒速，无足够曲线点才用s/"
           "v₀。负距离键仅用于插值0米速度，积分从0开始；曲线0米值与标称初速不一致时不缩放曲线。本模型把s近似为沿初始方"
           "向的推进距离，重力单独叠加，不冒充游戏三维阻力积分器。"),
        ready);
    add("inheritance", "子弹继承射手速度", integrated ? "conditional" : "agreed",
        tr("I = 0；只用自己的完整世界速度对齐相机位置。"), tr("用户指定，2026-10-16；不是全部载具已验证结论。"),
        tr("B(τ)=C_now+n·s(τ)+Gτ²/"
           "2，不加入V_self·τ。自己的速度只对齐陈旧相机样本并推进下一控制时刻的起点，不给已飞行的子弹重复平移。"),
        ready);
    add("zeroing", "全局归零", integrated ? "conditional" : "agreed",
        tr("%1 m，所有枪械共用。").arg(s.motion.zeroingDistanceMeters) +
            (ready ? tr("参考俯仰偏角 %1°；参考飞行 %2 ms。")
                         .arg(num(b.solution.zeroingPitchDegrees, 4), num(b.solution.zeroingSeconds * 1000, 2))
                   : Text(" · ") + ballisticStatus),
        Text("ballistics/zeroingDistanceMeters · domain/ballistics_settings.hpp"),
        tr("相机模式按手动100/200/300/400/"
           "500米，默认100米：参考点=C_now+视线·Z，以n_aim=R(n_zero→n_sight)·n_"
           "required扣等效归零。原生模式用已含原生归零的basis对D_launch，不再次扣手动零点。均不加入枪口/"
           "相机位置偏移。"),
        ready);
    const bool cameraPresent = s.view && s.view->camera && s.view->cameraOrigin && s.view->rotation &&
                               finite(*s.view->camera) && finite(*s.view->cameraOrigin) && finite(*s.view->rotation);
    add("origin", "弹道起点（相机）", integrated ? "conditional" : "agreed",
        b.originWorldMeters
            ? tr("相机起点世界XYZ %1 m；年龄 %2 ms。").arg(vector(*b.originWorldMeters), num(b.cameraAgeMs, 2))
        : cameraPresent ? tr("已选择Reader相机位置；等待源时间与速度对齐。")
                        : tr("等待有效相机位置与视角"),
        Text("Reader Local.camera.pose.location + origin + independent source time · "
             "games/pubg/pipeline/ballistic_aim.cpp"),
        tr("用户指定以当前相机为参考弹道起点，加自己的世界原点一次，按位置独立源时间与完整身体世界速度对齐。枪口位置不"
           "作起点；合格原生forward可单独作方向反馈。相机缺失或时钟不可靠时暂停。"),
        b.originWorldMeters.has_value());
    add("gravity", "有效重力与下坠", integrated ? "conditional" : "agreed",
        tr("重力 %1 m/s²，竖直向下。").arg(gravityText) +
            (ready ? tr("飞行下坠量 %1 m。").arg(num(b.solution.dropMeters, 4)) : Text(" · ") + ballisticStatus),
        Text("ballistics/gravityMetersPerSecondSquared · Aimbot settings · MotionConfiguration"),
        tr("G=(0,0,−g)，g为当前设置的重力加速度，范围0–100 "
           "m/s²，默认9.8。下坠位移为Gτ²/"
           "2；目标相遇、归零参考和20ms前馈使用同一g。设为0关闭重力下坠。此参数由用户设置，不是读取到的游戏重力，不乘未"
           "知阻力系数。"),
        ready);
    const auto weaponSource =
        catalog ? Text::fromStdString(catalog->sourcePath) + Text(" · ") + Text::fromStdString(catalog->revision)
                : Text("data/games/pubg/weapons/2026-10-15/profiles.json · published_reference.v1 / reference_only");
    add("weapon", "枪械与弹道资料", integrated ? "conditional" : "missing",
        profile ? tr("%1 · 标称初速 %2 m/s · 模型起点速度 %3 m/s。")
                      .arg(Text::fromStdString(profile->profile.name), num(profile->profile.initialSpeed, 2),
                           num(profile->curve.initialSpeed(), 2))
        : catalog ? tr("已载入 %1 条公开资料；等待匹配当前手持武器。").arg(catalog->entries.size())
                  : tr("弹道资料包未就绪"),
        weaponSource,
        tr("has_current_"
           "item为true只证明当前持有物品，不代表枪械或可开火；false为核验空手，缺失为未知。实时按本批当前武器的真实clas"
           "s_"
           "name匹配；资料库预留物品ID匹配，当前Reader投影未使用该字段。别名冲突且参数不同时拒绝，不按显示名猜枪。使用"
           "同一版本的基础公开参数，未按附件、特殊弹种或射击模式另加修正。保留reference_"
           "only来源身份；新资料整包验证后原子替换，同一revision不重复重置控制。"),
        profile && profile->curve.valid());

    const bool paired = s.living && s.view && checkPair(*s.living, *s.view, {s.sourceSkewMs, 800}).usable();
    add("frames", "Reader来源与场景配对", "wired",
        s.living && s.view ? tr("Living #%1 / Local #%2；%3")
                                 .arg(s.living->header.capture)
                                 .arg(s.view->header.capture)
                                 .arg(paired ? tr("配对可用") : tr("配对不可用"))
                           : waiting,
        Text("Reader Living/Local · games/pubg/pipeline/eligibility.cpp"),
        tr("只用CURRENT数据，核对Reader实例、epoch、World、Level、坐标约定、时钟域、原点映射、自己身份和控制器。连接任"
           "务启动不等于本帧可输出。"),
        paired);
    Text coordinates =
        target ? tr("相机到部位 %1 m").arg(num(target->error.distanceUnits / WorldUnitsPerMeter, 1)) : waiting;
    if (target && target->position && target->origin && s.view && s.view->camera && s.view->cameraOrigin)
    {
        const auto pos = prediction.applied() ? prediction.position : *target->position;
        const auto o = *target->origin, c = *s.view->camera, co = *s.view->cameraOrigin;
        const Vec3 point{(pos.x + o.x) / WorldUnitsPerMeter, (pos.y + o.y) / WorldUnitsPerMeter,
                         (pos.z + o.z) / WorldUnitsPerMeter};
        const Vec3 camera{(c.x + co.x) / WorldUnitsPerMeter, (c.y + co.y) / WorldUnitsPerMeter,
                          (c.z + co.z) / WorldUnitsPerMeter};
        coordinates += tr("；部位世界XYZ %1 m；相机世界XYZ %2 m。").arg(vector(point), vector(camera));
    }
    add("coordinates", "世界坐标、原点与距离", "wired", coordinates,
        Text("Reader camera / bone + own origin · control/aim_geometry.cpp"),
        tr("R = (P部位 + O部位) − (P相机 + "
           "O相机)，每个点只加自己的原点一次；使用XYZ三维距离。当前控制固定100游戏单位=1米，ESP的单位/"
           "宽高比选项不改变控制。"),
        target.has_value());
    const auto weaponAim = s.living ? resolveWeaponAim(*s.living, catalog.get(), s.motion.aimPart, s.motion.weaponAim)
                                    : WeaponAimSelection{s.motion.aimPart};
    add("part", "瞄准部位与骨点", "wired",
        Text(aimPartName(weaponAim.part)) + Text(" · ") + Text(std::string(aimBoneName(weaponAim.part))) + Text(" · ") +
            (weaponAim.category ? Text(std::string(weaponCategoryKey(*weaponAim.category))) : tr("未识别枪械类别")) +
            (weaponAim.overridden ? tr(" · 分类设置") : tr(" · 跟随默认")),
        Text("Reader semantic bones · games/pubg/pipeline/target_selector.cpp"),
        tr("头/颈/胸/腹/"
           "骨盆分别使用head、neck_01、spine_03、spine_"
           "01、pelvis；缺失或重名时不猜索引。仅头部旧样本兼容head位置字段。当前武器按严格资料映射选择SR/DMR/AR/SMG/"
           "LMG/"
           "Shotgun/Handgun/Other的独立设置；继承项和未知类别使用默认部位。SR按资料分类，包含Lynx "
           "AMR和Win94。控制、预选与诊断共用解析。"),
        target.has_value());
    add("selection", "候选筛选与优先级", "wired",
        (s.motion.targetPriority == TargetPriority::ClosestToCrosshair ? tr("准星距离优先") : tr("人物距离优先")) +
            Text(" · ") +
            tr("半角 %1°；距离上限 %2 m；合格 %3 人")
                .arg(num(TargetPolicy{}.maximumConeDegrees, 0), num(s.motion.maximumAimDistanceMeters, 1))
                .arg(d.selection.candidates),
        Text("games/pubg/pipeline/target_selector.cpp · Aimbot settings"),
        tr("排除自己、队友、缺队伍、缺部位和无效几何，通过角度与距离限制后排序。人物距离优先：先三维距离，再准星角差；"
           "准星距离优先：先所选部位与准星的角差，再三维距离；最后以人物ID稳定排序。只影响未锁定选人，不抢占本次按住的"
           "目标。没有额外按血量、枪械或可见性排序。"),
        paired);
    add("lock", "按住锁定与缺帧恢复", "wired",
        s.heldTarget
            ? tr("锁定人物ID %1；%2").arg(s.heldTarget->id).arg(s.suspended ? tr("暂停等待恢复") : tr("保留本次身份"))
            : tr("未锁定；下次按下选择合格目标。"),
        tr("games/pubg/pipeline/live_controller.cpp · Reader完整人物身份"),
        tr("一次按住固定id/actor/index/"
           "serial。暂缺、越界或短时无合格点时暂停并保留身份；恢复只续同一人，不补发断档位移。松键清锁；场景或配置变化"
           "须松开再按。"),
        s.heldTarget.has_value());
    add("timing", "采样差与源进展", "wired",
        tr("整帧Living/Local允许差 %1 ms；补偿采样差 %2 ms；源进展期限800 ms。")
            .arg(num(s.sourceSkewMs, 0), num(s.motion.livingLocalSkewMs, 0)),
        Text("games/pubg/pipeline/eligibility.cpp · games/pubg/pipeline/live_controller.cpp"),
        tr("类别采样时间差按用户设置检查；800ms检查的是本机观察到的源进展，不是精确跨机数据年龄。补偿另有更严格的年龄和"
           "时钟条件。"),
        paired);
    Text velocity = tr("当前没有可用目标速度。");
    if (target && target->bodyWorldVelocity)
        velocity = (target->bodyWorldVelocity->source == BodyVelocitySource::ReaderTotalWorld
                        ? tr("Reader完整世界速度")
                        : tr("原生组件速度；载具/附着增量按0")) +
                   Text(" · ") + vector(target->bodyWorldVelocity->value) + Text(" units/s");
    add("velocity", "目标速度来源", "conditional", velocity,
        Text("Reader Movement · games/pubg/model/control_frame.hpp · games/pubg/pipeline/latency_prediction.cpp"),
        tr("实时弹道只使用Reader本批完整身体世界速度，保留原生分支来源；骑乘/"
           "基座贡献已经在结果里，不再相加。组件速度仅独立保留诊断，完整速度缺失时不假设0、不沿用旧值；旧录制的组件估算"
           "不授予弹道资格。"),
        prediction.applied());
    add("prediction", "数据年龄补偿", "conditional",
        prediction.applied()
            ? tr("本帧使用：年龄 %1 ± %2 ms。").arg(num(prediction.ageMs, 1), num(prediction.uncertaintyMs, 1))
        : target
            ? (integrated ? tr("本帧尚不能完成时间对齐：%1；暂停弹道输出。") : tr("本帧未补偿，使用当前原始骨点：%1。"))
                  .arg(predictionReason(prediction.reason))
            : waiting,
        tr("Reader骨点/速度采样区间 + 四锚点时钟 · games/pubg/pipeline/latency_prediction.cpp"),
        tr("P当前 = P采样 + V × "
           "数据年龄。源区间中点映射到本机时钟；年龄+"
           "误差≤250ms、误差≤25ms、同一Living目标的骨点/速度采样差≤100ms，Local相机/"
           "Living己方速度采样差≤%1ms（用户设置）、探测年龄≤3000ms。失败不沿用旧速度。此处不含子弹飞行时间。")
            .arg(num(s.motion.livingLocalSkewMs, 0)),
        prediction.applied());
    const auto angleCurrent =
        target ? tr("水平误差 %1°；垂直误差 %2°。").arg(num(target->error.horizontal), num(target->error.vertical))
               : waiting;
    add("angles", "三维目标角度", "wired",
        angleCurrent + (ready ? tr("补偿后的世界视线方向 %1；发射方向 %2。")
                                    .arg(vector(b.solution.aimDirection), vector(b.solution.launchDirection))
                              : Text{}),
        tr("Reader最终相机pitch/yaw/roll · control/aim_geometry.cpp"),
        tr("筛选使用同一决策时刻的目标与相机位置；己方完整速度和相机时间合格时先对齐相机，避免乘车平移造成虚假角差。"
           "不修改相机旋转，不扩大瞄准范围，不绕过缺失速度的弹道校验。仍将R投影到完整相机forward/right/"
           "up，角锥由前向夹角计算。相机输出按D_aim和相机基向量求差；原生输出按预测basis与D_"
           "launch的球面切向误差，解控制器Yaw/Pitch两轴方向雅可比，不从forward伪造roll。"),
        target.has_value());

    const auto response =
        calibration ? tr("yaw %1°/count；pitch %2°/count。")
                          .arg(num(s.calibration.yawDegreesPerCount, 6), num(s.calibration.pitchDegreesPerCount, 6))
                    : waiting;
    add("mapping", "视角到鼠标计数", "conditional",
        (s.inputMapping == InputMapping::DynamicFov ? tr("实时FOV公式估算") : tr("仅使用匹配的实测记录")) +
            Text(" · ") + response,
        Text("games/pubg/pipeline/calibration_profiles.cpp · active calibration"),
        s.inputMapping == InputMapping::DynamicFov
            ? tr("yaw/count = 0.005 × 10^(S/50) × FOV/90；pitch/count = 符号 × yaw/count × "
                 "垂直倍率。属于输入响应估算，未声明所有模式精确。动态模式不混用实测值；仅实测模式要求模式/"
                 "FOV唯一匹配。")
            : tr("按用户选择的模式及实时FOV匹配唯一实测yaw/"
                 "pitch每计数角度，FOV容差0.01°；缺失或歧义则不输出，不偷偷使用动态公式。"),
        calibration);
    add("input", "实时FOV、灵敏度与DPI", "wired",
        tr("FOV %1°；模式 %2；DPI %3；常规/瞄准/ADS %4/%5/%6；垂直 %7。")
            .arg(s.view && s.view->fov ? num(*s.view->fov, 1) : Text("—"),
                 s.outputMode ? Text(modeName(*s.outputMode)) : tr("未指定"))
            .arg(s.motion.dpi)
            .arg(num(s.gameInput.general, 0), num(s.gameInput.aim, 0), num(s.gameInput.ads, 0),
                 num(s.gameInput.vertical)),
        tr("Reader实时FOV；游戏输入设置与用户选择的当前模式。"),
        tr("模式由用户指定，不靠FOV猜开镜状态；第三/"
           "第一人称用常规，瞄准用aim，开镜用ads。原始HID计数不再乘DPI；DPI参与学习速度尺度和校准配置。"),
        calibration);
    double speed = p.maximumCountsPerSecond, acceleration = p.acceleration, braking = p.acceleration;
    if (s.motion.model)
    {
        const auto scale = s.motion.dpi / 1600.0;
        speed = std::min(speed, s.motion.model->speed90 * scale);
        acceleration = std::min(acceleration, s.motion.model->acceleration90 * scale);
        braking = std::min(braking, s.motion.model->braking90 * scale);
    }
    add("speed", "五档速度与起停上限", "wired",
        tr("主动接近：速度 %1 counts/s，加速 %2，制动 %3 counts/s²；单轴步幅≤%4。锁定纠偏：速度 %5 counts/s，加速/制动 "
           "%6 counts/s²。")
            .arg(num(speed, 0), num(acceleration, 0), num(braking, 0))
            .arg(p.maximumStepCounts)
            .arg(num(p.maximumCountsPerSecond, 0), num(4 * std::min(p.acceleration, 250000.0), 0)),
        Text("control::speedPolicy · control/speed_policy.cpp"),
        tr("五档为当前链路每轴上限的18.75%/25%/50%/75%/100%。最高速度=min(单轴计数上限,floor(双轴预算/2))×1000/"
           "有效间隔；"
           "有效间隔同时满足固件能力和波特率下48字节START+MOTION的8N1传输时间。加速度与jerk按比例缩放，"
           "不放大采样模型。115200/500000自动探测，只有校验通过且序号前进的回传才确认连接；"
           "能力变化先停止并重新要求松键。物理鼠标保留1ms独立透传，不经PC控制队列；回传过期拒绝输出，无按住时长上限。"),
        s.controlActive);
    add("trajectory", "位移轨迹与整数化", "wired",
        tr("主动接近使用Ruckig，jerk上限 %1 counts/s³；近目标转入增益≤20/s的快速纠偏。").arg(num(p.maximumJerk, 0)),
        Text("Ruckig v0.15.3 · control/movement/trajectory_smooth.cpp"),
        tr("主动接近按当前速度/"
           "加速度重规划双轴限jerk轨迹；角差进入max(0.2°,1.5°×缩放)"
           "后，本次按住保持快速纠偏，不因每次后坐重启接近曲线。纠偏保留速度、加速度、单步和方向约束及整数余量，不使用"
           "随机路径。缺几何/时钟的旧来源保留有界回退。"),
        s.controlActive);
    add("feedforward", "目标角速度前馈", "conditional",
        ready                                       ? tr("使用下一决策时刻20ms后的弹道视线角差，不再次叠加飞行提前量。")
        : prediction.applied()                      ? tr("目标速度可用于20ms前向角差估算。")
        : target && !target->nativeMovementExpected ? tr("旧来源可使用有界骨点差分分支。")
                                                    : tr("等待可靠当前速度；实时来源缺速时不改用旧差分。"),
        Text("games/pubg/pipeline/motion_planner.cpp · target point / velocity"),
        tr("敌我共同推进20ms后重新解弹道；相机模式用D_aim，原生模式用D_launch。角速度低于1°/"
           "s不前馈、计数速度限于上限90%"
           "；跟随时使用该下一参考作为暂估生效目标。不是新增读帧延迟，不再叠数据年龄、旧骨点差分或第二次飞行提前量。"),
        (ready && b.aim && b.aim->nextPosition.has_value()) ||
            (!integrated && prediction.applied() && s.controlActive));
    add("learning", "学习模型参与范围", "conditional",
        s.motion.model ? tr("已启用模型；速度/加速/制动按DPI缩放后参与主动接近上限。")
                       : tr("未启用模型；仅载入或拟合不会参与输出。"),
        Text("MotionConfiguration.model · control/human_runtime.cpp"),
        tr("主动接近仅使用speed90、acceleration90、braking90，以DPI/"
           "1600缩放并与当前档位取较小值。锁定后的快速纠偏不受学习模型的慢起停限制。不学习命中能力、瞄准方向或残差轨迹"
           "。"),
        s.motion.model != nullptr);
    add("variation", "轻微随机扰动", "conditional",
        s.motion.variation ? tr("已允许有界横向扰动；接近目标时归零。") : tr("未启用随机扰动。"),
        Text("MotionConfiguration.variation · control/human_runtime.cpp"),
        tr("主动接近时60ms更新相关横向偏移，幅度有界约±0.2计数；角差≤0."
           "6°或已进入快速纠偏时不叠加，不累计随机游走。它是程序生成的微扰，不是学到的手抖。"),
        s.motion.variation);
    const auto rotationTiming = s.view
                                    ? estimateSampleTime(s.view->rotationSourceStartNs, s.view->rotationSourceFinishNs,
                                                         s.view->header.sourceClock, now)
                                    : SampleTimeEstimate{};
    const Text feedbackCurrent = feedback && feedback->timing.usable()
                                     ? tr("旋转数据年龄 %1 ± %2 ms；待反映 %3 项；角度外扰估计 yaw %4 / pitch %5 °/s。")
                                           .arg(num(feedback->timing.ageMs, 1), num(feedback->timing.uncertaintyMs, 1))
                                           .arg(feedback->pendingCommands)
                                           .arg(num(feedback->yawRate), num(feedback->pitchRate))
                                 : rotationTiming.usable()
                                     ? tr("当前旋转数据年龄 %1 ± %2 ms；当前未输出，尚无控制外扰估计。")
                                           .arg(num(rotationTiming.ageMs, 1), num(rotationTiming.uncertaintyMs, 1))
                                     : tr("等待当前控制反馈；旋转与相机位置各用自己的源采样时间。");
    add("feedback", "方向反馈与待反映位移", "wired", feedbackCurrent,
        tr("互斥的原生basis或最终相机 + 已提交计数 · MotionPlanner::reconcile"),
        tr("只用按下时选定参考的独立源时间，10至100ms按实龄处理；年龄含误差≤250ms、误差≤25ms。暂估20ms输入响应核算待反"
           "映命令，不把后坐角当回执。扣预计自身输入后，用160ms窗、至少80ms覆盖估计整体外扰；低于max(0.75°/s,2计数/"
           "窗)归零，每轴≤45°/s，外推年龄+20ms且每轴≤1.5°。原生模式不叠相机残差。缺时钟/"
           "断档清预测暂停，不补发积压；残差不是纯后坐或设备ACK。"),
        s.controlActive);
    const double zoom = s.calibration.formulaEstimated ? std::min(1.0, s.calibration.fov / 80) : 1;
    add("settling", "近目标稳定区", "wired",
        calibration ? tr("静止进入 %1°，离开 %2°；当前FOV缩放 %3。")
                          .arg(num(std::max(0.06, p.deadZoneDegrees) * zoom, 4),
                               num(std::max(0.12, p.deadZoneDegrees * 2) * zoom, 4), num(zoom, 3))
                    : waiting,
        Text("games/pubg/pipeline/motion_planner.cpp · mapping / FOV"),
        tr("静止目标在0.06°进入、0.12°离开稳定区，清速度、加速度、余量与微扰；动态公式按min(1,FOV/"
           "80)缩放。各轴死区0.05°同缩放，有角速度前馈时保持跟随。"),
        s.controlActive);
    add("output", "触发、输出与停止条件", "wired",
        s.controlActive ? tr("当前正在跟随。") : tr("当前没有输出；链路就绪后仍须物理侧键触发。"),
        Text("control/output_mode.hpp · button_feedback.hpp · src/application/runtime/output.cpp"),
        tr("Reader请求与盒子连接后自动待命；侧键回传需≤150ms且能力有效。相机、完整速度、时钟或武器资料缺失时暂停。松键"
           "停止，调度间隔>30ms停止，采集/校准/"
           "桌面测试独占。串口有限单步、配置代次和板端松键门控继续检查，不自动开火。"),
        s.controlActive);

    add("flight_lead", "子弹飞行时间提前量", integrated ? "conditional" : "excluded",
        ready ? tr("目标命中时XYZ %1 m；数值模型残差 %2 m。")
                    .arg(vector(b.solution.targetAtImpact), num(b.solution.residualMeters, 7))
              : ballisticStatus,
        Text("control/ballistics.cpp · games/pubg/pipeline/ballistic_aim.cpp"),
        tr("各源先独立对齐到当前决策时刻，暂设额外击发延迟δ=0；再解||R+V_enemy·τ−Gτ²/"
           "2||=s且τ=T(s)。192个递增距离区间取首个正解，再至多64次二分、距离阈值1e−7米，正向代回残差须≤0."
           "002米。优先遵守资料中的射程；未知射程用10km数值搜索界，飞行搜索最多10秒，这些是求解界而非实测游戏寿命。"),
        ready);
    add("acceleration", "目标加速度与局部骨骼运动", "excluded", tr("未建模；数据年龄补偿按短时恒速平移。"),
        Text("games/pubg/pipeline/latency_prediction.cpp"),
        tr("未加入目标加速度、突然转向、头部相对身体动画速度、载具转动造成的骨点速度、相机绕身体转动或局部动画位移。身"
           "体线速度用于短时恒速平移，不冒称每个骨点或相机的完整瞬时速度。"));
    add("shot_effects", "独立后坐力、散布与环境", "conditional",
        nativeActive ? tr("使用原生方向整体反馈与有界外扰预测，不另叠呼吸角或剩余后坐。")
                     : tr("使用相机角度外扰预测；等待合格原生方向，独立角补偿未启用。"),
        tr("games/pubg/pipeline/motion_planner.cpp；Reader原生目标输入量不作为已施加角。"),
        tr("原生模式追踪散布前方向整体变化，不逐项相加呼吸、后坐或相机震动；相机模式保留原反馈。两模式互斥，不要求开火"
           "才校正。不使用枪械压枪表、未来随机散布、风、空气密度或伤害倍率；输入按下不等于实际出膛，原生方向也不是每颗"
           "随机弹的最终方向。"),
        feedback && feedback->residualReady);
    add("visibility", "遮挡、命中与开火判定", "excluded", tr("未做弹道碰撞、可见性射线、血量判定或自动开火。"),
        tr("当前目标选择器与输出链路的功能边界。"),
        tr("ESP显示筛选不改变Aimbot资格；几何合格不表示无遮挡或子弹必定命中，当前程序只修正鼠标位移。"));

    const auto telemetry = describeReaderTelemetry(s.view, now, nativeActive);
    const bool consumer = s.view && s.view->header.contract == control::FrameContract::ConsumerV3;
    const auto raw = s.view && s.view->controllerRotation ? &*s.view->controllerRotation : nullptr;
    const auto w = s.view && s.view->activeWeapon ? &*s.view->activeWeapon : nullptr;
    const auto targets = w && w->recoilTargets ? &*w->recoilTargets : nullptr;
    const auto generation = w && w->trajectoryGeneration ? &*w->trajectoryGeneration : nullptr;
    const auto preciseRotation = [&](Vec3 value) {
        return Text("(%1, %2, %3)").arg(num(value.x, 6), num(value.y, 6), num(value.z, 6));
    };
    const auto observation = [&](Vec3 value, const Json &sample) {
        const auto age = sample["timing_usable"].get<bool>() ? tr("源年龄 %1 ± %2 ms。")
                                                                   .arg(num(sample["age_ms"].get<double>(), 3),
                                                                        num(sample["uncertainty_ms"].get<double>(), 3))
                                                             : tr("源时钟暂不可用；保留原始观察。");
        return tr("pitch/yaw/roll %1°。 ").arg(preciseRotation(value)) + age;
    };
    add("camera_rotation", "最终相机旋转", "observed",
        s.view && s.view->rotation ? observation(*s.view->rotation, telemetry["camera_rotation"]) : waiting,
        Text("Reader Local.camera.pose.rotation · own rotation-field source interval"),
        tr("最终相机原始三轴六位显示，保留独立源时间；此处可无输出观察，相机参考模式使用它反馈，ESP和筛选继续使用它。原"
           "生反馈模式不再用其残差纠偏。显示精度不是测量精度，异时相减不证明纯呼吸。"),
        s.view && s.view->rotation.has_value());
    add("native_rotation", "原生控制器旋转", nativeActive ? "wired" : "observed",
        raw ? observation(raw->rotation, telemetry["controller_rotation"]) : waiting,
        Text("Reader Local.controller_rotation · GetControlRotation · own source interval"),
        tr("原始控制器FRotator独立采样，原生方向控制用其yaw确定Pitch输入轴，不与相机相减猜呼吸。角度与计数的关系沿用当"
           "前校准近似；不是最终发射方向，也不伪造枪口roll。"),
        raw != nullptr);
    const auto optionalNumber = [&](const std::optional<double> &v) { return v ? num(*v, 5) : Text("—"); };
    add("native_recoil_targets", "原生剩余后坐输入量", "observed",
        targets
            ? tr("pitch %1 / yaw %2 / recovery %3；原生输入量，非度数。")
                  .arg(optionalNumber(targets->pitch), optionalNumber(targets->yaw), optionalNumber(targets->recovery))
            : waiting,
        consumer ? Text("Reader Consumer.weapon.recoil · sources[5]")
                 : Text("Reader Local.current_weapon.weapon.recoil_targets · NestedSample [5,5]"),
        tr("当前身体和武器生命周期内的剩余目标输入量，随后逐帧消费、恢复并进入控制器输入缩放。目标对与恢复量分别读取，"
           "源区间不代表原子三轴。缺失不填零，不作为已施加后坐角再次扣除；未参与控制。"),
        targets != nullptr);
    add("native_trajectory_generation", "原生轨迹生成计数", "observed",
        generation ? tr("原始uint32计数 %1；不推导持续开火状态。").arg(generation->counter) : waiting,
        consumer ? Text("Reader Consumer.weapon.trajectory_generation · sources[6]")
                 : Text("Reader Local.current_weapon.weapon.trajectory_generation · NestedSample [5,6]"),
        tr("建立本地轨迹记录时递增，霰弹可能一次生成多条轨迹，不等于扳机次数、弹药发数、命中或服务器确认。只显示原始uin"
           "t32及独立源时间，允许回绕；未派生开火事件、射速或精确发生时刻。未来若解读变化，初样本、缺失、换枪、重连及身"
           "份变化须重建基准。"),
        generation != nullptr);
    const auto holding = s.view && s.view->nativeHolding ? &*s.view->nativeHolding : nullptr;
    const auto sway = s.view && s.view->nativeSway ? &*s.view->nativeSway : nullptr;
    const auto basis = s.view && s.view->nativeAimBasis ? &*s.view->nativeAimBasis : nullptr;
    const auto ageText = [&](const Json &sample) {
        return sample["timing_usable"].get<bool>()
                   ? tr("源年龄 %1 ± %2 ms。")
                         .arg(num(sample["age_ms"].get<double>(), 3), num(sample["uncertainty_ms"].get<double>(), 3))
                   : tr("源时钟暂不可用；保留原始观察。");
    };
    const auto providedStatus = [&](const Json &value) {
        return value["provided"].get<bool>()
                   ? tr("本组终态：%1；无当前有效值。").arg(value["status"].get<std::string>())
                   : waiting;
    };
    add("native_holding", "原生屏息状态与过渡", "observed",
        holding && (holding->accepted || holding->blend)
            ? tr("accepted %1 / blend %2；两个字段独立采样。")
                      .arg(holding->accepted ? Text(*holding->accepted ? "true" : "false") : Text("—"),
                           optionalNumber(holding->blend)) +
                  Text(" ") +
                  tr("接受状态：%1 过渡因子：%2")
                      .arg(ageText(telemetry["native_holding"]["accepted_sample"]),
                           ageText(telemetry["native_holding"]["blend_sample"]))
            : providedStatus(telemetry["native_holding"]),
        Text("Reader Local.native_holding · accepted/blend · own field sources"),
        tr("原生动作组件实际接受状态及过渡因子；false和0是真实值。各字段保留自己源区间，组件归属由Reader核验，合法失败"
           "只清本组。屏"
           "息不是统一乘1-H，blend为1也不证明全部晃动归零；仅诊断。"),
        holding && (holding->accepted || holding->blend));
    add("native_sway", "原生三轴混合晃动", "observed",
        sway && sway->rotation ? observation(*sway->rotation, telemetry["native_sway"]["rotation_sample"])
                               : providedStatus(telemetry["native_sway"]),
        Text("Reader Local.native_sway · GetCurrentSway · interpretation origin=2"),
        tr("本批真实输入执行已核验原生本地曲线得到的FRotator，单位度，可能混合运动、姿态、配件和屏息。不是纯呼吸角；源"
           "区间覆盖实际输入，不借相机时间，不再加到已含几何姿态的basis；仅诊断。"),
        sway && sway->rotation.has_value());
    const bool viewNoTrace = basis && basis->branch == 2;
    add("native_aim_basis", "原生散布前方向基准", nativeActive && !viewNoTrace ? "wired" : "observed",
        basis && basis->direction
            ? (viewNoTrace
                   ? tr("无射线视线forward %1；未施加原生归零，仅观察。").arg(preciseRotation(*basis->direction))
                   : tr("世界forward %1；原生归零 %2 m / %3°。")
                         .arg(preciseRotation(*basis->direction), optionalNumber(basis->zeroingDistanceMeters),
                              optionalNumber(basis->zeroingCorrectionDegrees))) +
                  Text(" ") + ageText(telemetry["aim_basis"]["direction_sample"]) + Text(" ") +
                  (viewNoTrace
                       ? Text{}
                       : tr("积分记录：%1 步 / 到达 %2 / %3 cm。")
                             .arg(basis->integratedSteps ? num(*basis->integratedSteps, 0) : Text("—"),
                                  basis->reachedDistance ? Text(*basis->reachedDistance ? "true" : "false") : Text("—"),
                                  optionalNumber(basis->integratedDistanceCm)))
            : providedStatus(telemetry["aim_basis"]),
        Text("Reader Local.aim_basis · post_branch_pre_spread · ") +
            Text::fromStdString(basis ? basis->nativeProfile : ""),
        tr("BARREL已含原生归零，合格时与D_launch作整体反馈。VIEW_NO_"
           "TRACE仅观察原生明确不做物理射线的视线方向，未加归零，没有Mesh或轨迹配置，不进入控制或沿用barrel比较。均不重"
           "加sway/剩余后坐，也不代表未来随机单发方向。"),
        basis && basis->direction.has_value());
    const auto comparison = describeNativeAimComparison(s.view, b, now);
    add("breath_reference", "原生方向到输入的映射", "conditional",
        nativeActive ? tr("原生方向反馈正在使用；输入映射为近似模型，未做游戏实测。")
        : comparison["available"].get<bool>()
            ? tr("原生方向只读夹角 %1°；满足输入资格后可在下次按下时使用。")
                  .arg(num(comparison["observed_launch_separation_degrees"].get<double>(), 6))
            : tr("当前使用相机参考，等待可用的原生方向与控制器输入轴。"),
        Text::fromUtf8(NativeInputModel.data(), NativeInputModel.size()),
        tr("按单步内局部枪口姿态不变近似，用世界Yaw轴和控制器Yaw确定的Pitch轴建立方向雅可比，解二轴输入，再沿用当前每计"
           "数角度。没有推断roll；两源中点差不超过25ms。以独立方向源时间处理年龄、待反映命令和整体外扰，不叠加相机残差"
           "。参考在按下时选定，原生暂失暂停保留人物，恢复清预测；不是实机响应保证。"),
        d.nativeAim.reference.has_value());

    const auto group = [](const char *id, const char *title, const Json &ids) {
        return Json{{"id", id}, {"title", tr(title)}, {"ids", ids}};
    };
    Json sections{
        group(
            "ballistics", "弹道约定与接入状态",
            {"shot_direction", "flight_rule", "inheritance", "gravity", "zeroing", "origin", "weapon", "flight_lead"}),
        group("geometry", "数据、目标与三维角度",
              {"frames", "coordinates", "part", "selection", "lock", "timing", "velocity", "prediction", "angles"}),
        group("motion", "输入换算与鼠标控制",
              {"mapping", "input", "speed", "trajectory", "feedforward", "learning", "variation", "feedback",
               "shot_effects", "settling", "output"}),
        group("telemetry", "原生遥测与方向反馈",
              {"camera_rotation", "native_rotation", "native_recoil_targets", "native_trajectory_generation",
               "native_holding", "native_sway", "native_aim_basis", "breath_reference"}),
        group("excluded", "当前未参与的计算", {"acceleration", "visibility"})};
    return {
        sections,
        {{"revision",
          Text::fromUtf8(assumptions.revision.data(), static_cast<std::size_t>(assumptions.revision.size()))},
         {"ballisticsIntegrated", assumptions.liveBallisticsIntegrated},
         {"summary", integrated ? (nativeActive ? tr("当前链路：独立源时间对齐 → 三维飞行相遇 → D_launch → "
                                                     "原生方向预测与二轴输入。原生暂失暂停，不切换参考。")
                                                : tr("当前链路：独立源时间对齐 → 三维飞行相遇 → 下坠与等效归零 → "
                                                     "相机反馈。下次按下时优先选择合格原生方向。"))
                                : tr("当前输出：几何跟随 + 有条件的数据年龄补偿。子弹飞行、下坠与归零尚未接入。")},
         {"assumptionSummary",
          tr("弹道约定：相机起点 · 基础同向 + 等效归零 · 公开曲线求积 · 不继承射手速度 · 重力 %1 m/s²。")
              .arg(gravityText)},
         {"note",
          tr("当前值来自最近一次状态快照；“已接入”表示链路包含此项，不表示此刻正在输出。用户约定保留假设身份。")},
         {"facts", facts},
         {"controlActive", s.controlActive},
         {"generation", std::to_string(s.generation)},
         {"reader_telemetry", telemetry},
         {"timing_metadata", describeTimingMetadata(s, d, now)},
         {"native_aim_comparison", comparison},
         {"native_input_model", Json{{"name", NativeInputModel},
                                     {"used_for_control", nativeActive},
                                     {"eligible", d.nativeAim.reference.has_value()},
                                     {"qualification_reason", static_cast<int>(d.nativeAim.block)},
                                     {"verified_in_game", false},
                                     {"sensitivity_source", "existing_calibration_model_assumption"},
                                     {"local_barrel_pose", "frozen_for_each_small_command"},
                                     {"maximum_controller_basis_skew_ms", 25},
                                     {"roll_inferred", false},
                                     {"sway_added", false},
                                     {"recoil_targets_added", false},
                                     {"zeroing_subtracted_again", false}}},
         {"rotation_sample",
          Json{{"available", rotationTiming.usable()},
               {"reason", static_cast<int>(rotationTiming.reason)},
               {"age_ms", rotationTiming.usable() ? Json(rotationTiming.ageMs) : Json{}},
               {"uncertainty_ms", rotationTiming.usable() ? Json(rotationTiming.uncertaintyMs) : Json{}},
               {"source_start_ns", s.view && s.view->rotationSourceStartNs > 0
                                       ? Json(std::to_string(s.view->rotationSourceStartNs))
                                       : Json{}},
               {"source_finish_ns", s.view && s.view->rotationSourceFinishNs > 0
                                        ? Json(std::to_string(s.view->rotationSourceFinishNs))
                                        : Json{}}}},
         {"angular_feedback",
          Json{
              {"reference", nativeActive ? "native_barrel_basis" : "camera"},
              {"source_timed", feedback && feedback->timing.usable()},
              {"tracking", feedback && feedback->tracking},
              {"residual_ready", feedback && feedback->residualReady},
              {"age_ms", feedback && feedback->timing.usable() ? Json(feedback->timing.ageMs) : Json{}},
              {"uncertainty_ms", feedback && feedback->timing.usable() ? Json(feedback->timing.uncertaintyMs) : Json{}},
              {"pending_commands", feedback ? Json(feedback->pendingCommands) : Json{}},
              {"yaw_rate_degrees_per_second", feedback && feedback->residualReady ? Json(feedback->yawRate) : Json{}},
              {"pitch_rate_degrees_per_second",
               feedback && feedback->residualReady ? Json(feedback->pitchRate) : Json{}},
              {"yaw_lead_degrees", feedback && feedback->residualReady ? Json(feedback->yawLead) : Json{}},
              {"pitch_lead_degrees", feedback && feedback->residualReady ? Json(feedback->pitchLead) : Json{}},
              {"response_model_ms", AngularFeedbackTuning::responseMs},
              {"response_model_measured", false},
              {"residual_window_ms", AngularFeedbackTuning::windowMs},
              {"minimum_residual_window_ms", AngularFeedbackTuning::minimumWindowMs},
              {"minimum_rate_degrees_per_second", AngularFeedbackTuning::minimumRateDegreesPerSecond},
              {"maximum_rate_degrees_per_second", AngularFeedbackTuning::maximumRateDegreesPerSecond},
              {"maximum_lead_degrees", AngularFeedbackTuning::maximumLeadDegrees},
              {"residual_is_pure_recoil", false},
              {"device_ack", false}}},
         {"ballistics", Json{{"ready", ready},
                             {"status", ballisticStatus},
                             {"profile_id", Text::fromStdString(b.profileId)},
                             {"weapon_class", Text::fromStdString(b.weaponClass)},
                             {"pack_revision", catalog ? Text::fromStdString(catalog->revision) : Text{}},
                             {"source_path", catalog ? Text::fromStdString(catalog->sourcePath) : Text{}},
                             {"flight_seconds", ready ? Json(b.solution.flightSeconds) : Json{}},
                             {"drop_meters", ready ? Json(b.solution.dropMeters) : Json{}},
                             {"zeroing_pitch_degrees", ready ? Json(b.solution.zeroingPitchDegrees) : Json{}},
                             {"gravity_m_per_s2", ready ? Json(b.gravityMetersPerSecondSquared) : Json{}},
                             {"residual_meters", ready ? Json(b.solution.residualMeters) : Json{}},
                             {"aim_direction", ready ? vector(b.solution.aimDirection) : Text{}},
                             {"launch_direction", ready ? vector(b.solution.launchDirection) : Text{}},
                             {"zeroing_reference_direction", ready ? vector(b.solution.zeroingDirection) : Text{}},
                             {"target_age_ms", ready ? Json(b.targetAgeMs) : Json{}},
                             {"camera_age_ms", b.originWorldMeters ? Json(b.cameraAgeMs) : Json{}},
                             {"origin", "reader_camera"},
                             {"origin_world_m", b.originWorldMeters ? Json(vector(*b.originWorldMeters)) : Json{}},
                             {"timing_part", static_cast<int>(b.timingPart)},
                             {"timing_reason", b.timingReason ? Json(static_cast<int>(*b.timingReason)) : Json{}}}},
         {"assumptions", Json{{"trajectory_origin", "reader_camera"},
                              {"muzzle_geometry_used", false},
                              {"shot_direction", "same_as_aim_line_before_equivalent_zeroing"},
                              {"zeroing_rule", "equivalent_reference_rotation"},
                              {"flight_time_rule", "published_reciprocal_speed_trapezoid"},
                              {"inherited_shooter_velocity", assumptions.inheritedShooterVelocity},
                              {"gravity_m_per_s2", gravity},
                              {"gravity_source", "user_setting"},
                              {"gravity_default_m_per_s2", assumptions.defaultGravityMetersPerSecondSquared},
                              {"provenance", "user_decision"}}}}};
}
} // namespace mousebox::application
