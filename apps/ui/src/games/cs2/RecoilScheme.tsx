import {RecoilScheme, type RecoilFactor} from "~/features/aim/common/RecoilScheme";
import {useTranslation} from "react-i18next";

const factors: readonly RecoilFactor[] = [
  {title: "可预测后坐 P", fields: ["C_CSPlayerPawn::m_pAimPunchServices",
    "CCSPlayer_AimPunchServices::m_predictableBaseAngle", "CCSPlayer_AimPunchServices::m_predictableBaseAngleVel",
    "CCSPlayer_AimPunchServices::m_predictableBaseTick", "CCSPlayer_AimPunchServices::m_predictableBaseTickInterpAmount"],
  detail: "使用后坐基准角、基准角速度及 tick/fraction，在本批原生模拟时间上按冻结 E 函数求值，包含原生衰减、积分、插值与缩放。"},
  {title: "不可预测后坐 U", fields: ["CCSPlayer_AimPunchServices::m_unpredictableBaseAngle",
    "CCSPlayer_AimPunchServices::m_unpredictableBaseTick"],
  detail: "使用已读取的不可预测后坐基准与时间求值，再与 P 相加。外推采用已经收到的射击或受击冲量。"},
  {title: "后坐求值时间", fields: ["AimPunchState.times.simulation"],
  detail: "在已核验的本批模拟时间上求出 P/U，执行时刻的外推采用 Aim 常规中的时序设置。"},
];

export function Cs2RecoilScheme() {
  const {t} = useTranslation();
  return <RecoilScheme name="原生后坐求值" factors={factors}
    help={<p>{t("使用已核验的原生 P/U 后坐计算。目标选择、移动轨迹和速度变化控制在各自模块处理。")}</p>}/>;
}
