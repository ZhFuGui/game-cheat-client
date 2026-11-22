#include "controller_state.hpp"
#include <cmath>
#include "control_messages.hpp"
namespace mousebox::games::cs2
{
using application::Json;
void describeController(Json &snapshot, const ControllerState &r, bool eligible, bool diagnostics, bool selected)
{
    const auto vector = [](const std::optional<Angles> &a) -> Json {
        return a ? Json::array({a->x, a->y, a->z}) : Json{};
    };
    auto &state = snapshot["state"];
    const auto &s = r.solution;
    const bool ready = selected && s.ready() && r.inputMappingReady && eligible && !r.calibrating;
    state["canAim"] = ready;
    state["aimArmed"] = ready;
    state["aimAutomatic"] = true;
    state["aimReference"] = s.direction ? s.directionReference == "global-input" ? "input" : "native" : "pending";
    state["canCalibrate"] = selected && eligible && r.calibrationInput.ready();
    state["calibrating"] = r.calibrating;
    state["target"] = s.target ? Json(s.targetName) : Json{};
    state["angle"] = s.error ? Json(std::hypot(s.error->x, s.error->y)) : Json{};
    const auto block = !selected ? "rage-selected" : !eligible ? "session-ineligible" : r.block;
    state["control"] = controlMessage(block);
    state["cs2Controller"] = {
        {"block", block},
        {"recoilOnly", selected && eligible && r.recoilOnly},
        {"version", r.version},
        {"modeId", r.modeId},
        {"comparisonFactor", r.comparisonFactor},
        {"directionReference", s.directionReference},
        {"motionStatus", r.motionStatus},
        {"movementAlgorithm", r.movementAlgorithm},
        {"active", selected && eligible && r.active},
        {"down", r.down},
        {"releaseRequired", r.releaseRequired},
        {"calibrated", r.calibrated},
        {"inputMapping", r.inputMapping},
        {"inputMappingReady", r.inputMappingReady},
        {"countScaleX", r.inputMappingReady ? Json(r.countScaleX) : Json{}},
        {"countScaleY", r.inputMappingReady ? Json(r.countScaleY) : Json{}},
        {"model", r.model},
        {"variation", r.variation},
        {"targetController", s.target ? Json(std::to_string(s.target->controller)) : Json{}},
        {"targetPawn", s.target ? Json(std::to_string(s.target->pawn)) : Json{}},
        {"targetName", s.targetName},
        {"distanceUnits", s.target ? Json(s.distanceUnits) : Json{}},
        {"sampleAgeMs", s.timing.usable() ? Json(s.timing.ageMs) : Json{}},
        {"uncertaintyMs", s.timing.usable() ? Json(s.timing.uncertaintyMs) : Json{}},
        {"horizonMs", s.timing.usable() ? Json(s.horizonMs) : Json{}},
        {"recoilHorizonMs", s.timing.usable() ? Json(s.recoilHorizonMs) : Json{}},
        {"desired", vector(s.desired)},
        {"direction", vector(s.direction)},
        {"error", vector(s.error)},
        {"yawPerCount", r.inputMappingReady ? Json(r.yawPerCount) : Json{}},
        {"pitchPerCount", r.inputMappingReady ? Json(r.pitchPerCount) : Json{}},
        {"pendingCommands", r.pending},
        {"heldTargetController", r.heldTarget ? Json(std::to_string(r.heldTarget->controller)) : Json{}},
        {"heldTargetPawn", r.heldTarget ? Json(std::to_string(r.heldTarget->pawn)) : Json{}},
        {"heldTargetName", r.heldTargetName},
        {"suspended", r.suspended},
        {"lastPause", r.lastPause},
        {"lastPauseAgeMs", r.lastPauseAgeMs ? Json(*r.lastPauseAgeMs) : Json{}},
        {"pauseCount", std::to_string(r.pauseCount)},
        {"releaseReason", r.releaseReason},
        {"feedbackReference", r.feedbackReference},
        {"calibrationStatus", r.calibrationStatus},
        {"calibrationLeg", r.calibrationLeg},
        {"calibrating", r.calibrating},
        {"calibrationBlock", r.calibrationInput.block},
        {"calibrationSampleAgeMs", r.calibrationInput.timing.usable() ? Json(r.calibrationInput.timing.ageMs) : Json{}},
        {"calibrationUncertaintyMs",
         r.calibrationInput.timing.usable() ? Json(r.calibrationInput.timing.uncertaintyMs) : Json{}},
        {"calibrationResult", r.calibrationResult ? calibrationJson(*r.calibrationResult) : Json{}}};
    if (!diagnostics)
        return;
    auto &facts = snapshot["algorithm"]["facts"];
    const auto add = [&](const char *id, const char *title, const char *principle) {
        facts[id] = {{"id", id},
                     {"title", title},
                     {"current", "CS2 native controller"},
                     {"source", "正式 Reader v2 + 本项目控制器"},
                     {"principle", principle},
                     {"status", "implemented"},
                     {"phase", "wired"}};
    };
    add("cs2-control-target", "选人与锁定",
        "按 Aim 常规筛选存活、非休眠且所选 Hitbox 姿态有效的人物。初选受视野和角度约束，按住保留完整"
        "人物句柄。瞄点定时在内缩命中体内取样，随当前姿态移动，掩体判断复用当前瞄点的预测位置。");
    add("cs2-control-time", "执行时刻估计",
        "时钟映射的读取区间中点年龄加手动执行延迟；人物与己方速度按同一时间平移。P/"
        "U在本批游戏时间上外推相同秒数，假设期间没有新冲量或时间缩放；未知未来射击/"
        "受击不能预测。过期、不确定或未知时钟不输出。");
    add("cs2-control-angle", "角度求解",
        "Source "
        "2坐标：yaw=atan2(dy,dx)，pitch=-atan2(dz,hypot(dx,dy))。所需角减已核验分支的eye_basis及发射P/"
        "U，yaw取最短角差。关闭补偿时仅减eye_basis；不把相机0.45或view-punch当弹道，不重复加入矩阵。");
    add("cs2-control-count", "设备计数换算",
        "默认原生公式明确采用设备计数到原生输入增量1:"
        "1的链路假设，并非实测；直接使用有符号导数，不重复乘灵敏度、纵轴或开镜倍率。实测模式使用用户已采用的独立横纵比"
        "值，没有有效记录就停止，不回退成公式或假装已测。");
    add("cs2-control-calibration", "可选设备校验",
        "校准仅比较原生输入角度、鼠标导数和采样时间，不要求敌人、眼角基准、后坐或弹道状态。四向有限步始终由用户主动按住"
        "侧键；缺失、过期、时钟不可信仍停止并显示具体原因。结果保存成功后再切到实测模式，不自动采用。");
    add("cs2-control-motion", "鼠标运动",
        "速度档位为当前握手上限比例；每步、间隔、总XY预算均受设备能力约束。加减速及停止距离限制，保留小数计数；启用学习"
        "模型时进一步限制速度/加减速。未被采样时刻反映的已发角度先从误差扣除，这是假设响应模型而非ACK。");
    add("cs2-control-version", "瞄准与后坐方案",
        "保留v1–v4历史瞄准与后坐方案；其原移动方式只在选择沿用历史方案时应用。鼠标移动可独立选择，"
        "不改变原生后坐求值或方向参考。单变量组固定v1资格和目标及平滑基线，只改变登记的一项因素。"
        "两项选择分别按游戏保存，切换清理旧状态，实机效果由用户比较。");
    facts["cs2-control-version"]["current"] = r.modeId;
    facts["cs2-control-motion"]["current"] = r.movementAlgorithm == "direct"            ? "直接量化"
                                             : r.movementAlgorithm == "recoil-tracking" ? "平滑跟随＋后坐快速通道"
                                                                                        : "平滑跟随";
    facts["cs2-control-motion"]["source"] = "control/movement · " + r.movementAlgorithm;
    if (r.movementAlgorithm == "recoil-tracking")
        facts["cs2-control-motion"]["principle"] =
            "后坐变化优先使用独立计数通道，平滑跟随只处理扣除该部分后的剩余角差，全部已接受位移共用账本，避免重复。"
            "首次锁定或时间路线不连续时重建差分参考；非普通本地预测输入及关闭补偿时回到基线运动。"
            "拒绝提议回滚快速状态和分数预算，不重置已经接受的跟随状态；真实侧键、会话与来源门禁保持。";
    if (r.movementAlgorithm == "direct")
        facts["cs2-control-motion"]["principle"] =
            "完整剩余角差在扣除待反馈位移后直接按最近计数量化，跳过比例追赶、加减速与学习微扰；"
            "保留当前速度、学习模型速度上限、死区和设备预算。小数残差保留在当前角差，拒绝提议退还速率预算。";
    if (s.directionReference == "global-input")
        facts["cs2-control-angle"]["principle"] =
            "方向计算使用Local.input_view_angles（dwViewAngles）加当前P/U；这不是Reader已求值的原生eye_basis。"
            "v4同时移除原生眼角资格与响应依赖；单变量方向组仅替换角差参考，保留v1资格、选人和响应观察。";
    if (!r.comparisonFactor.empty())
    {
        add("cs2-control-comparison", "单变量对照组",
            "对照组固定使用完整v1数据完成资格与选人，再对同一当前目标仅改变登记的一项策略；"
            "不放宽字段缺失条件，不叠加v2/v3改动。没有触发条件时无差异不代表因素无作用；"
            "P/U原生E内部定义、会话/侧键/身份和设备能力固定。具体改变、字段及有效条件见版本模块。");
        facts["cs2-control-comparison"]["current"] = r.modeId;
        facts["cs2-control-comparison"]["source"] = r.comparisonFactor;
    }
    add("cs2-control-stop", "资格与停止",
        "仅普通本地第一人称、当前眼角与后坐分支核验通过且换算可用时输出。已证明普通本地source2输入角不依赖视觉选择器，"
        "允许"
        "它与后坐的编号不同；其他分支仍须匹配。模拟时间可因预测路线切换而回退，不将它当跨批单调时钟；每批完整求值，"
        "采集序号和QPC仍单独核验。短暂缺组、采集乱序或调度中断只暂停同人，当前可信数据恢复后继续；"
        "侧键回传短暂中断时，仅有现有设备松开计数能证明未松键才恢复原锁定；真实松键结束锁定。"
        "反馈优先核对普通本地source2的Pawn输入角，真正无响应时保留目标并等待新的实际角度变化，不自动试探移动。"
        "Reader、时钟域、epoch、进程、世界或本地人物身份"
        "变"
        "化、会话撤销和设备失效则停止并要求松键。物理侧键新鲜且按住才发有限步；来源5–7的局部鼠标变换未核验不输出。没有"
        "自"
        "动开火、散布或PUBG弹道模型。");
    if (r.version == "v4")
        facts["cs2-control-stop"]["principle"] =
            "v4要求普通本地角色、全局输入角、完整P/U和可信时间；控制与响应都不依赖eye_basis/Pawn.v_angle。"
            "采用全局输入角加P/U的实验方向模型，不声称等同所有原生父变换或历史路线。身份、侧键、死亡与设备约束不变。";
    if (!r.comparisonFactor.empty())
    {
        facts["cs2-control-motion"]["principle"] =
            "单变量组使用同一个v1运动内核，只修改登记的一个因子；若变量属于运动模块，仅关闭对应限制或微扰。"
            "其他运动因素、设备预算与输出资格固定；具体条件见版本模块。";
        facts["cs2-control-time"]["principle"] =
            "读取年龄资格保持v1；采样年龄、预计执行延迟和P/U求值推进为独立因子。对照只改变选定项，"
            "不修改账本的响应配置；运行面板分别展示几何与后坐窗口。";
        snapshot["algorithmSections"].push_back(
            {{"title", "单变量对照"}, {"scope", "aimbot"}, {"ids", Json::array({"cs2-control-comparison"})}});
    }
    snapshot["algorithm"]["summary"] = "CS2 独立跟随控制器；设备实测与闭环需用户验收";
    snapshot["algorithm"]["revision"] = "cs2-controller-" + r.modeId + "/" + r.movementAlgorithm;
    snapshot["algorithm"]["mouseSummary"] = "有符号角差按明确的原生1:"
                                            "1公式或已采用实测比值换算；所有方式受握手速度、步长和预算约束。鼠标移动独"
                                            "立选择，原生后坐与瞄准求值保持对应方案。";
    facts["cs2-input"]["current"] = r.inputMapping == "native" ? "原生 1:1 公式（未实测）"
                                    : r.calibrated             ? "已采用实测换算"
                                                               : "实测模式等待有效记录";
    snapshot["algorithmSections"].push_back(
        {{"title", "CS2 跟随算法"},
         {"scope", "aimbot"},
         {"ids", Json::array({"cs2-control-version", "cs2-control-target", "cs2-control-time", "cs2-control-angle",
                              "cs2-control-stop"})}});
    snapshot["algorithmSections"].push_back(
        {{"title", "CS2 鼠标移动"},
         {"scope", "mouse"},
         {"ids", Json::array({"cs2-control-count", "cs2-control-motion", "cs2-control-calibration"})}});
}
} // namespace mousebox::games::cs2
