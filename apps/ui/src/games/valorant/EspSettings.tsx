import {useTranslation} from "react-i18next";
import {Link} from "react-router";
import {SettingChoice,SettingField,SettingToggle} from "~/components/shared/SettingField";
import {HelpTooltip} from "~/components/shared/HelpTooltip";
import {Button} from "~/components/ui/button";
import {useCommand} from "~/hooks/useCommand";
import {useRuntimeStore} from "~/runtime/state";
import type {GameEspSettingsProps} from "../contracts";

export function EspSettings({tab,setTab}:GameEspSettingsProps){
  const {t}=useTranslation(),{run,pending}=useCommand();
  const scale=useRuntimeStore(s=>s.values?.valorantMetersPerUnit??0);
  return <aside className="esp-settings" aria-label={t("ESP 配置")}>
    <div className="tabs" role="group" aria-label={t("视图")}>{["display","projection"].map(value=><button key={value} className={tab===value?"active":""} aria-pressed={tab===value}
      onClick={()=>setTab(value)}>{t(value==="display"?"显示":"投影")}</button>)}</div>
    <div className="esp-settings-scroll">{tab==="display"?<>
      <SettingChoice esp setting="marker" label="标记样式" options={[{label:"定位点",value:"point"},{label:"骨点范围角标",value:"corners"}]}/>
      <SettingToggle esp setting="names" label="人物编号" help={t("显示 Reader 实体编号，不是人物名称。")}/>
      <SettingToggle esp setting="bones" label="骨骼" help={t("仅画本批有效的 22 点及已知父子连线。骨点范围不是 Hitbox，缺点不补旧值；位置或骨骼单路缺失时，另一有效流仍可显示。")}/>
      <SettingToggle esp setting="distance" label="距离" help={t("距离从当前相机到人物根坐标计算；缺根坐标时使用当前有效骨点。未设置米制比例时以原生单位 u 显示。")}/>
      <SettingToggle esp setting="tracers" label="射线"/>
      <SettingToggle esp setting="crosshair" label="中心准星"/>
      <SettingField esp setting="maximumDistanceMeters" label="显示距离上限" suffix="m" disabled={scale<=0}
        help={t("0 表示不限。米制比例在游戏设置中明确设置后可用。")}/>
      <Link to="/visuals?section=general">{t("人物筛选")}</Link>
    </>:<>
      <div className="field-label"><strong>{t("相机重建投影")}</strong><HelpTooltip label="相机重建投影">{t("按 UE 相机坐标约定解释欧拉角及 FOV，原生画幅约束生效时保持对应比例。这是客户端重建，不是游戏最终投影矩阵；缩放或特殊视口需人工核对。")}</HelpTooltip></div>
      <SettingChoice esp setting="aspect" label="投影视口" options={["viewport","16:9","16:10","21:9","4:3"].map(value=>({label:value==="viewport"?"相机约束或当前区域":value,value}))}/>
      <SettingChoice esp setting="fovAxis" label="FOV 解释" presentation="buttons" options={[{label:"水平",value:"horizontal"},{label:"垂直",value:"vertical"}]}/>
      <Link to="/game-settings?section=valorant-display">{t("窗口与单位设置")}</Link>
    </>}</div>
    <div className="esp-panel-footer"><Button variant="outline" className="full-width" disabled={pending}
      onClick={()=>void run("esp.reset",{})}>{t("恢复默认")}</Button></div>
  </aside>;
}
