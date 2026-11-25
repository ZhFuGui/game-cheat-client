import { useTranslation } from "react-i18next";
import { cs2SettingsSchema } from "@mousebox/shared";
import { SettingField, SettingChoice, SettingToggle } from "~/components/shared/SettingField";
import { Button } from "~/components/ui/button";
import { useRuntimeStore } from "~/runtime/state";
import { useCommand } from "~/hooks/useCommand";
import type { GameEspSettingsProps } from "../contracts";
import { ExtensionSlot } from "~/features/extensions/ExtensionSlot";
import { HitboxStatus } from "./HitboxStatus";

export function EspSettings({ tab, setTab }: GameEspSettingsProps) {
  const { t } = useTranslation(), { run, pending } = useCommand();
  const scale = useRuntimeStore(store => store.values?.cs2MetersPerUnit ?? 0);
  const reset = async () => {
    if (!(await run("esp.reset", {}))) return;
    if (!(await run("settings.set", {
      key: "cs2ShowHealth", value: cs2SettingsSchema.shape.cs2ShowHealth.parse(undefined),
    }))) return;
    await run("settings.set", {
      key: "cs2HitboxDisplay", value: cs2SettingsSchema.shape.cs2HitboxDisplay.parse(undefined),
    });
  };
  return <aside className="esp-settings" aria-label={t("ESP 配置")}>
    <div className="tabs" role="group" aria-label={t("视图")}>
      {["display", "projection"].map(value => <button key={value} className={tab === value ? "active" : ""} aria-pressed={tab === value} onClick={() => setTab(value)}>{t(value === "display" ? "显示" : "投影")}</button>)}
    </div>
    <div className="esp-settings-scroll">
      {tab === "display" ? <>
        <SettingChoice esp setting="marker" label="标记样式" options={[{ label: "定位点", value: "point" }, { label: "骨点范围角标", value: "corners" }]} />
        <SettingToggle esp setting="names" label="名称" />
        <SettingToggle setting="cs2ShowHealth" label="血量" />
        <SettingToggle esp setting="bones" label="原始骨点" help={<><p>{t("骨点与连线仅来自 Reader 本批已核验模型；连线使用显式父子边，缺点不补齐。角标仅表示已读骨点范围。")}</p><p>{t("HEAD 表示头部骨原点，不代表头顶或命中盒中心；未知模型或来源不可用时不显示骨架。")}</p></>} />
        <SettingChoice setting="cs2HitboxDisplay" label="Hitbox 命中体" options={[
          { label: "关闭", value: "off" }, { label: "实体命中体", value: "solid" }, { label: "线框命中体", value: "wireframe" },
        ]}  help={<> <p>{t("命中体尺寸来自 CS2 人物资源，随本批骨骼世界姿态更新；缺失部位不补旧帧。")}</p> </>}/>
        <HitboxStatus />
        <SettingToggle esp setting="distance" label="距离" disabled={scale <= 0} />
        <SettingToggle esp setting="tracers" label="射线" />
        <SettingToggle esp setting="crosshair" label="中心准星" />
        <hr />
        <SettingToggle esp setting="teammates" label="队友" />
        <SettingToggle esp setting="opponents" label="其他玩家" />
        <SettingToggle esp setting="self" label="自己" />
        <ExtensionSlot name="esp-settings" />
        <SettingField esp setting="maximumDistanceMeters" label="显示距离上限" suffix="m" slider={{min:0,max:1000,step:5}} disabled={scale <= 0} help={t("米制比例在游戏设置中指定。未设置时不根据未知单位显示距离。")}/>
      </> : <>
        <SettingChoice esp setting="aspect" label="原始游戏视口比例" options={["16:9", "16:10", "21:9", "4:3", "viewport"].map(value => ({ label: value === "viewport" ? "填满区域" : value, value }))}  help={<> <p>{t("直接使用 Reader 世界到裁剪矩阵；视口比例需与游戏画面一致，不另外解释 FOV。")}</p><p>{t("米制比例在游戏设置中指定。未设置时不根据未知单位显示距离。")}</p> </>}/>
      </>}
    </div>
    <div className="esp-panel-footer">
      <Button variant="outline" className="full-width" disabled={pending} onClick={() => void reset()}>{t("恢复默认")}</Button>
    </div>
  </aside>;
}
