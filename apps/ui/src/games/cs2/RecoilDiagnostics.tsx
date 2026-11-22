import { useTranslation } from "react-i18next";
import type { Cs2RecoilTelemetry } from "@mousebox/shared";
import { Section, Hint, display } from "~/components/shared/Content";

type Row = [string, unknown];
function format(value: unknown): string {
  if (Array.isArray(value)) return value.map(n => typeof n === "number" ? n.toFixed(5) : display(n)).join(" / ");
  if (value && typeof value === "object" && "tick" in value && "fraction" in value) {
    return `${display(value.tick)} + ${display(value.fraction)}`;
  }
  return display(value);
}
export function RecoilDiagnostics({ value }: { value: Cs2RecoilTelemetry | null | undefined }) {
  const { t } = useTranslation();
  const p = value?.localPunch, c = value?.cameraPunch, v = value?.cameraViewState;
  const mapping = value?.mouse, result = value?.evaluated;
  const source = mapping?.source === 1 ? "鼠标" : mapping?.source === 2 ? "模拟输入" : "未知";
  const path = value?.cameraPath === 1 ? "本地 Player" : value?.cameraPath === 2 ? "观战 Player" : "未知";
  const eyeSource = ["未知", "网络直接返回", "预测输入", "原生插值缓存", "原生历史求值", "父变换后的输入", "视图委托", "实体世界旋转"];
  const rows: Row[] = [
    ["原生输入来源", t(source)], ["相机后坐路径", t(path)], ["相机来源完整句柄", value?.cameraPawn],
    ["横轴度/原生增量", mapping?.yawDegreesPerDelta], ["纵轴度/原生增量", mapping?.pitchDegreesPerDelta],
    ["纵轴灵敏度倍率", mapping?.verticalScale], ["原始 m_yaw / m_pitch", mapping ? [mapping.yaw, mapping.pitch] : null],
    ["有效 pitch 系数", mapping?.effectivePitch], ["Y 轴反转", mapping?.invertedY == null ? null : t(mapping.invertedY ? "是" : "否")],
    ["原始 zoom ratio", mapping?.zoomRatio], ["Pawn FOV 倍率", mapping?.pawnFovAdjust],
    ["Pawn 鼠标覆盖值（0 为无覆盖）", mapping?.pawnMouseSensitivity],
    ["跟随后坐准星", value?.crosshairEnabled == null ? null : t(value.crosshairEnabled ? "开启" : "关闭")],
    ["本地 P 基准 tick + fraction", p?.predictable?.baseTime], ["本地 P 基础角速度（度/秒）", p?.predictable?.baseAngularVelocity],
    ["本地 U 基准 tick", p?.unpredictable?.baseTick], ["本地 WorldGroup", p?.times?.worldGroup],
    ["本地网络眼角（度）", value?.networkEyeAngles], ["Pawn 输入眼角（度）", value?.inputEyeAngles],
    ["本地已核验眼角基准（度）", value?.localEyeBasis?.value],
    ["本地眼角基准来源", t(eyeSource[value?.localEyeBasis?.source ?? 0] ?? "未知")],
    ["眼角原生上下文", value?.localEyeBasis?.nativeContext], ["后坐原生上下文", p?.times?.nativeContext],
    ["眼角与后坐上下文", value?.localBasisContextMatches == null ? null : t(value.localBasisContextMatches ? "一致" : "不一致")],
    ["射击方向上下文规则", t(value?.directionContextIndependent ? "已核验本地输入分支不依赖该编号" : "当前分支要求上下文匹配")],
    ["眼角内部插值查询参数", value?.localEyeBasis?.interpolationParameter],
    ["委托末端分支", value?.localEyeBasis?.resolvedSource == null ? null : t(eyeSource[value.localEyeBasis.resolvedSource] ?? "未知")],
    ["委托来源完整句柄", value?.localEyeBasis?.sourceEntity], ["委托跳数",value?.localEyeBasis?.delegationDepth],
    ["本地 simulation tick + fraction", p?.times?.simulation], ["每 tick 秒数", p?.times?.secondsPerTick],
    ["本批射击 P 求值（度）", result?.shotPredictable], ["本批射击 U 求值（度）", result?.shotUnpredictable],
    ["本批射击偏移 P + U（度）", result?.shotOffset], ["网络眼角 + 本批后坐（非武器服务方向）", result?.rawEyePlusRecoil],
    ["已核验分支方向推导（非未来开火）", result?.contextDirection],
    ["本地跟随后坐准星 P（度）", result?.crosshairFollow],
    ["相机 P 基准 tick + fraction", c?.predictable?.baseTime], ["相机 U 基准 tick", c?.unpredictable?.baseTick],
    ["相机 P 基角（度）", c?.predictable?.baseAngle], ["相机 P 基础角速度（度/秒）", c?.predictable?.baseAngularVelocity],
    ["相机 U 基角（度）", c?.unpredictable?.baseAngle],
    ["相机视觉时间策略", c?.times?.visualPolicy], ["相机 visual P tick + fraction", c?.times?.visualPredictable],
    ["相机 visual U tick + fraction", c?.times?.visualUnpredictable], ["相机 WorldGroup", c?.times?.worldGroup],
    ["相机原生上下文", c?.times?.nativeContext], ["相机来源眼角基准（度）", value?.cameraEyeBasis?.value],
    ["相机眼角来源", t(eyeSource[value?.cameraEyeBasis?.source ?? 0] ?? "未知")],
    ["相机委托末端分支", value?.cameraEyeBasis?.resolvedSource == null ? null : t(eyeSource[value.cameraEyeBasis.resolvedSource] ?? "未知")],
    ["相机委托来源句柄",value?.cameraEyeBasis?.sourceEntity], ["相机委托跳数",value?.cameraEyeBasis?.delegationDepth],
    ["相机 AimPunch 的 0.45 分量（度）", result?.cameraAim],
    ["独立 view-punch 基准 tick + fraction", v?.baseTime], ["独立 view-punch 求值秒时间", v?.evaluationSeconds],
    ["独立 view-punch 基角（度）", v?.baseAngle],
    ["独立 view-punch 实际衰减参数", v?.decayExponent], ["独立 view-punch 求值（度）", result?.cameraView],
    ["已知相机反馈之和（度）", result?.cameraFeedback],
  ];
  return <Section title="CS2 原生映射与后坐求值（只读）" help={<> <p>{t("仅按本批原生游戏时刻求值；不是未来开火方向，不生成鼠标输入。缺失显示 —，真实零值保留。")}</p><p>{t("协议眼角为原始网络字段；该派生对照不等于引擎 EyeAngles、真实武器方向或未来开火方向。")}</p><p>{t("已核验的普通本地输入分支不依赖该上下文编号；其他分支仍要求同批匹配。缺失眼角基准、后坐或时间不代填，也不回退到网络眼角。")}</p><p>{t("历史、父变换和委托角度均已由 Reader 求值，不再重复变换；委托来源仅解释基准，不替换本人物的后坐所有者。")}</p><p>{t("度/原生增量已包含轴向符号、灵敏度和开镜倍率，不再重复相乘；0 系数不能作为除数，也不代表设备已校准。")}</p><p>{t("相机视图矩阵已含渲染效果，求值结果不再叠加到 ESP；观战来源不等于本地人物，未知路径不补旧值。")}</p><p>{t("Reader 单发、连发、停火、换枪、变倍、受击和第一人称观战记录已核对；其他模式、设备校准和 CS2 跟随闭环尚未验收。")}</p><p>{t("字段与矩阵不是原子同刻，相机还有其他效果；不得把简化角度对照当作最终渲染相机。")}</p> </>}>
    {value && !value.buildSupported && <Hint>{t("当前 Build 未核验，保留原始状态但不执行后坐求值。")}</Hint>}
    <dl className="details-grid">{rows.map(([label, item]) => <div key={label}><dt>{t(label)}</dt><dd>{format(item)}</dd></div>)}</dl>
  </Section>;
}
