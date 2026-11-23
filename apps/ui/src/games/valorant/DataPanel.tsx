import { useTranslation } from "react-i18next";
import { Section } from "~/components/shared/Content";
import { useRuntimeStore } from "~/runtime/state";

const number = (value: number | null | undefined) => value == null ? "—" : value.toFixed(3);
const vector = (value: number[] | null | undefined) => value ? value.map(number).join(" / ") : "—";

export function DataPanel() {
  const {t} = useTranslation();
  const streams = useRuntimeStore(state => state.state.valorant?.streams);
  const context = useRuntimeStore(state=>state.state.valorant?.context);
  const localLabels={unknown:"未知",absent:"无受控人物",unresolved:"等待关联实体",resolved:"已关联实体"};
  return <div className="settings-stack">
    {context&&<Section title="场景信息" help={t("本地人物来自实际控制关系，相机与观战目标不用于推断自己。原生队伍 0 是有效值，缺失保持未知。") }>
      <dl className="details-grid">
        <div><dt>{t("当前地图")}</dt><dd>{context.map?.worldName??"—"}</dd></div>
        <div><dt>{t("地图包路径")}</dt><dd>{context.map?.packagePath??"—"}</dd></div>
        <div><dt>{t("本地人物关联")}</dt><dd>{t(localLabels[context.local.state])}</dd></div>
        <div><dt>{t("本地实体编号")}</dt><dd>{context.local.entityId??"—"}</dd></div>
        <div><dt>{t("本地参照队伍")}</dt><dd>{context.referenceTeam??"—"}</dd></div>
      </dl>
    </Section>}
    {(streams ?? []).map(stream => <Section key={stream.stream} title={["", "人物位置", "人物骨骼", "相机"][stream.stream]}
      help={t("位置、骨骼和相机保留各自的采样序号和时间，缺失值不沿用。实体身份按 Reader 实例与代次关联。") }>
      <dl className="details-grid">
        <div><dt>{t("状态")}</dt><dd>{t(stream.current ? "数据已接收" : stream.connected ? "等待订阅数据" : stream.connecting ? "正在连接" : "未连接")}</dd></div>
        <div><dt>{t("已确认订阅频率")}</dt><dd>{stream.subscribedHz == null ? "—" : `${stream.subscribedHz} Hz`}</dd></div>
        <div><dt>{t("Reader 实例")}</dt><dd>{stream.reader || "—"}</dd></div>
        <div><dt>{t("来源代次")}</dt><dd>{stream.epoch ?? "—"}</dd></div>
        <div><dt>{t("采样序号")}</dt><dd>{stream.capture ?? "—"}</dd></div>
        {stream.stream !== 3 && <div><dt>{t("人物记录数")}</dt><dd>{stream.entities ?? "—"}</dd></div>}
        {stream.stream === 3 && <>
          <div><dt>{t("相机位置")}</dt><dd>{vector(stream.camera?.position)}</dd></div>
          <div><dt>{t("相机角度")}</dt><dd>{vector(stream.camera?.rotation)}</dd></div>
          <div><dt>{t("原生 FOV")}</dt><dd>{number(stream.camera?.fov)}</dd></div>
          <div><dt>{t("相机画幅比例")}</dt><dd>{number(stream.camera?.aspect)}</dd></div>
          <div><dt>{t("投影类型")}</dt><dd>{t(stream.camera?.projection === 1 ? "透视" : stream.camera?.projection === 2 ? "正交" : "未知")}</dd></div>
        </>}
      </dl>
      {stream.error && <p className="field-error" role="status">{stream.error}</p>}
    </Section>)}
    {!streams && <p className="hint">{t("等待当前订阅数据")}</p>}
  </div>;
}
