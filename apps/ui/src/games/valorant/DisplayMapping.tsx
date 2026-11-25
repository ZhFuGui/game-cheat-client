import {useTranslation} from "react-i18next";
import {Section} from "~/components/shared/Content";
import {SettingField} from "~/components/shared/SettingField";
import {useRuntimeStore} from "~/runtime/state";

export default function DisplayMapping(){
  const {t}=useTranslation();
  const camera=useRuntimeStore(s=>s.state.valorant?.streams.find(stream=>stream.stream===3)?.camera);
  return <div className="settings-stack valorant-display-mapping">
    <Section title="本机游戏窗口" help={t("叠加层按本机进程名查找前台窗口，不使用远端游戏 PID。Reader 位于远端时，可继续使用内嵌 ESP；本机叠加层只能覆盖本机游戏。") }>
      <SettingField setting="valorantWindowExecutable" numeric={false} label="游戏进程文件名"/>
    </Section>
    <Section title="世界单位" help={t("0 表示米制比例未知，距离按 Reader 原生单位 u 显示。只有明确填入米/原生单位比例后才换算为 m 并允许米制距离筛选。") }>
      <SettingField setting="valorantMetersPerUnit" label="每原生单位对应米数" slider={{min:0,max:0.1,step:0.0001}}/>
    </Section>
    <Section title="当前相机" help={t("ESP 使用当前相机位置、欧拉角和原生镜头参数重建投影，不是 Reader 提供的最终屏幕矩阵。可在 ESP 投影页调整视口比例和 FOV 轴向。未知投影和缺失相机不会沿用旧画面。") }>
      <dl className="details-grid">
        <div><dt>{t("投影类型")}</dt><dd>{t(camera?.projection===1?"透视":camera?.projection===2?"正交":"未知")}</dd></div>
        <div><dt>{t("原生 FOV")}</dt><dd>{camera?.fov??"—"}</dd></div>
        <div><dt>{t("相机画幅比例")}</dt><dd>{camera?.aspect??"—"}</dd></div>
      </dl>
    </Section>
  </div>;
}
