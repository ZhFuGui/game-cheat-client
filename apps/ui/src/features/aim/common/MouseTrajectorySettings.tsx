import {useTranslation} from "react-i18next";
import {SettingChoice, SettingField} from "~/components/shared/SettingField";
import {useRuntimeStore} from "~/runtime/state";

export function MouseTrajectoryChoice({disabled}: {disabled: boolean}) {
  const {t} = useTranslation();
  return <SettingChoice setting="mouseTrajectory" label="移动轨迹" disabled={disabled}
    options={[{value: "linear", label: "直线"}, {value: "bezier", label: "贝塞尔曲线"}]}
    help={t("直线朝目标推进。贝塞尔曲线由起点、终点和两个控制点决定；速度变化控制单独决定沿路径推进的节奏。")}/>;
}

export function MouseBezierSettings({disabled}: {disabled: boolean}) {
  const {t} = useTranslation();
  const mode = useRuntimeStore(s => s.values?.mouseTrajectory);
  const firstX = useRuntimeStore(s => s.values?.mouseBezierControl1X ?? 25);
  const firstY = useRuntimeStore(s => s.values?.mouseBezierControl1Y ?? 20);
  const secondX = useRuntimeStore(s => s.values?.mouseBezierControl2X ?? 75);
  const secondY = useRuntimeStore(s => s.values?.mouseBezierControl2Y ?? 20);
  if (mode !== "bezier") return null;
  const minY = Math.min(0, firstY, secondY) - 10;
  const height = Math.max(35, Math.max(0, firstY, secondY) + 10 - minY);
  return <div className="mouse-bezier-settings aim-full-row">
    <svg role="img" aria-label={t("曲线预览")} className="mouse-bezier-preview"
      viewBox={`-10 ${minY} 120 ${height}`}>
      <line x1="0" y1="0" x2="100" y2="0" className="bezier-chord"/>
      <path d={`M 0 0 L ${firstX} ${firstY} M ${secondX} ${secondY} L 100 0`} className="bezier-handles"/>
      <path d={`M 0 0 C ${firstX} ${firstY}, ${secondX} ${secondY}, 100 0`} className="bezier-curve"/>
      <circle cx="0" cy="0" r="1.5"/><circle cx="100" cy="0" r="1.5"/>
      <circle cx={firstX} cy={firstY} r="1.4" className="bezier-control"/>
      <circle cx={secondX} cy={secondY} r="1.4" className="bezier-control"/>
    </svg>
    <div className="aim-fields">
      <SettingField setting="mouseBezierControl1X" label="控制点 1 · 沿程" suffix="%" disabled={disabled}
        help={t("0–100%。沿起点到终点方向的位置，按两点距离计算。控制点只塑造曲线，不要求经过此点。")}/>
      <SettingField setting="mouseBezierControl1Y" label="控制点 1 · 偏移" suffix="%" disabled={disabled}
        help={t("-100–100%。垂直于起点到终点方向的偏移；正负值决定弯曲方向。两个偏移同号形成弧线，异号可形成 S 形，均为 0 时沿直线。")}/>
      <SettingField setting="mouseBezierControl2X" label="控制点 2 · 沿程" suffix="%" disabled={disabled}
        help={t("0–100%。沿起点到终点方向的位置，按两点距离计算。控制点只塑造曲线，不要求经过此点。")}/>
      <SettingField setting="mouseBezierControl2Y" label="控制点 2 · 偏移" suffix="%" disabled={disabled}
        help={t("-100–100%。垂直于起点到终点方向的偏移；正负值决定弯曲方向。两个偏移同号形成弧线，异号可形成 S 形，均为 0 时沿直线。")}/>
    </div>
  </div>;
}
