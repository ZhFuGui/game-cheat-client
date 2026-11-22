import { useTranslation } from "react-i18next";
import { Section, Hint, display } from "~/components/shared/Content";
import { useRuntimeStore } from "~/runtime/state";

const branches = { normal: "普通求值", forced: "强制散布", "no-spread": "关闭散布", "no-owner": "已确认无拥有者", unavailable: "未求值" };
const statuses = { valid: "有效", unavailable: "不可用", unverified: "未核验", inconsistent: "来源不一致" };
const number = (value: number | null | undefined) => value == null ? "—" : value.toFixed(7);

export function AccuracyDiagnostics() {
  const { t } = useTranslation();
  const value = useRuntimeStore(state => state.state.cs2Accuracy);
  const status = (result: NonNullable<typeof value>["inaccuracy"] | undefined) =>
    result ? `${t(statuses[result.status])}${result.reason ? ` · ${t(result.reason)}` : ""}` : t("等待当前 CS2 数据");
  return <Section title="原生散布" className="accuracy-diagnostics" help={<>
    <p>{t("Inaccuracy 和 Spread 都是无量纲方向幅度，不是角度、像素或命中率；数值越小，方向分散通常越小。")}</p>
    <p>{t("完整 Inaccuracy 已包含游戏更新后的精度惩罚和转向项，再结合移动、空中和条件方向项求值；Spread 独立显示，不重复相加。")}</p>
    <p>{t("仅展示本批已核验结果，不用于瞄准或开火判断。缺失显示 —，有效的 0 保留。")}</p>
  </>}>
    <dl className="details-grid">
      <div><dt>{t("完整 Inaccuracy（无量纲）")}</dt><dd>{number(value?.inaccuracy.value)}</dd></div>
      <div><dt>{t("独立 Spread（无量纲）")}</dt><dd>{number(value?.spread.value)}</dd></div>
      <div><dt>{t("Inaccuracy 状态")}</dt><dd>{status(value?.inaccuracy)}</dd></div>
      <div><dt>{t("Spread 状态")}</dt><dd>{status(value?.spread)}</dd></div>
      <div><dt>{t("原生求值分支")}</dt><dd>{value ? t(branches[value.branch]) : "—"}</dd></div>
      <div><dt>{t("核验模型")}</dt><dd>{display(value?.modelId)}</dd></div>
      {(["penalty", "movement", "airborne", "turning", "lateral"] as const).map((key, index) =>
        <div key={key}><dt>{t(["已更新精度惩罚", "移动贡献", "空中贡献", "已更新转向贡献", "方向贡献"][index])}</dt><dd>{number(value?.components?.[key])}</dd></div>)}
      <div><dt>{t("散布数据读取区间")}</dt><dd>{value?.readIntervalMs == null ? "—" : `${value.readIntervalMs.toFixed(3)} ms`}</dd></div>
    </dl>
    <Hint>{value ? `${t("当前采样")} #${value.capture} · ${t("武器")} ${value.weapon ?? "—"} · ${t("拥有者")} ${value.owner ?? "—"}` : t("等待当前 CS2 数据")}</Hint>
  </Section>;
}
