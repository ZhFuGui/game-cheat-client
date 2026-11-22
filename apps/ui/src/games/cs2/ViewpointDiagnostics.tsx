import { useTranslation } from "react-i18next";
import { Section, Hint } from "~/components/shared/Content";
import { useRuntimeStore } from "~/runtime/state";

const number = (value: number | null | undefined) => value == null ? "—" : value.toFixed(4);
const angle = (value: number | null | undefined) => value == null ? "—" : `${number(value)} °`;
export function ViewpointDiagnostics() {
  const { t } = useTranslation();
  const value = useRuntimeStore(state => state.state.cs2Viewpoint);
  const rows = [
    { label: "Reader 视点坐标（原始）", vector: value?.readerEyePosition },
    { label: "人物世界坐标", vector: value?.bodyPosition },
    { label: "视点偏移（原始）", vector: value?.rawViewOffset },
  ];
  return <Section title="视点与姿态" className="viewpoint-diagnostics" help={<> <p>{t("坐标使用游戏原生单位；角度按俯仰 / 水平 / 翻滚排列。缺失数据显示 —。")}</p><p>{t("Reader 原始视点由人物原点与原始偏移相加；已求值的原生视点尚未下发。")}</p> </>}>
    <dl className="details-grid viewpoint-angles">
      <div><dt>{t("全局输入俯仰角")}</dt><dd>{angle(value?.inputAngles?.[0])}</dd></div>
      <div><dt>{t("全局输入水平角")}</dt><dd>{angle(value?.inputAngles?.[1])}</dd></div>
      <div><dt>{t("人物输入俯仰角")}</dt><dd>{angle(value?.pawnInputAngles?.[0])}</dd></div>
      <div><dt>{t("人物输入水平角")}</dt><dd>{angle(value?.pawnInputAngles?.[1])}</dd></div>
      <div><dt>{t("身体实际旋转")}</dt><dd>{value?.bodyRotation ? `${value.bodyRotation.map(n => number(n)).join(" / ")} °` : "—"}</dd></div>
      <div><dt>{t("身体实际缩放")}</dt><dd>{number(value?.bodyScale)}</dd></div>
    </dl>
    <div className="viewpoint-coordinates">
      <table aria-label={t("视点坐标对照")}>
        <thead><tr><th scope="col">{t("项目")}</th>{["X", "Y", "Z"].map(axis => <th key={axis} scope="col">{axis}</th>)}</tr></thead>
        <tbody>{rows.map(row => <tr key={row.label}><th scope="row">{t(row.label)}</th>{[0, 1, 2].map(index => <td key={index}>{number(row.vector?.[index])}</td>)}</tr>)}
          {["已求值原生视点", "已选视点偏移"].map(label => <tr key={label}><th scope="row">{t(label)}</th><td colSpan={3}>{t("Reader 未提供")}</td></tr>)}
        </tbody>
      </table>
    </div>
    <Hint>{value ? `${t("当前采样")} #${value.capture} · ${t("本地人物")} ${value.pawn ?? "—"}${value.bodyMatched ? "" : ` · ${t("人物主体尚未匹配")}`}` : t("等待当前 CS2 数据")}</Hint>
  </Section>;
}
