import {useState} from "react";
import {useTranslation} from "react-i18next";
import {mouseMovementDefinitions} from "@mousebox/shared";
import {FeatureSection} from "~/components/shared/FeatureSection";
import {SettingChoice,SettingField} from "~/components/shared/SettingField";
import {Button} from "~/components/ui/button";
import {MouseMovementAlgorithms} from "~/features/aim/common/MouseMovementAlgorithms";
import {MovementSpeedFields} from "~/features/aim/common/MovementSpeedFields";
import {useRuntimeStore} from "~/runtime/state";
import {useCommand} from "~/hooks/useCommand";

export function AimSettings(){
  const {t}=useTranslation();
  const state=useRuntimeStore(store=>store.state.valorantAim),{run}=useCommand();
  const [busy,setBusy]=useState(false);
  const confirm=async()=>{
    setBusy(true);
    try{await run("game.valorant.aim.confirmView",{confirmed:!state?.manualViewConfirmed});}
    finally{setBusy(false);}
  };
  return <>
    <FeatureSection title="跟随设置" help={t("按住侧键后保持同一目标，短暂缺帧或部位缺失时暂停并等待恢复。使用头、颈、胸或骨盆的当前骨点，不生成 Hitbox 或掩体判断。") }>
      <div className="aim-fields">
        <SettingChoice setting="valorantAimPart" label="瞄准部位" options={["头部","颈部","胸部","骨盆"].map((label,value)=>({label,value}))}/>
        <SettingChoice setting="valorantTargetPriority" label="目标优先级" options={[{label:"人物距离优先",value:0},{label:"准星距离优先",value:1}]}/>
        <SettingChoice setting="trigger" label="触发侧键" options={[{label:"后退侧键",value:8},{label:"前进侧键",value:16}]}/>
        <SettingField setting="valorantAimCone" label="最大瞄准角差" suffix="°" slider={{min:0.1,max:89,step:0.1}}/>
        <SettingField setting="valorantAimDistanceUnits" label="最大目标距离（原生单位）" slider={{min:0,max:30000,step:100}}
          help={t("0 表示不限距离，按相机到目标骨点的原生世界单位计算。")}/>
      </div>
    </FeatureSection>
    <FeatureSection title="手动视角确认" help={t("Reader 尚未提供相机归属与可控状态。请只在本地第一人称、自己可以控制视角时确认；确认仅属于当前连接、Reader 会话及本地人物，重新连接或换人后失效。观战、死亡或改变视角模式后请撤销确认。") }>
      <div className="aim-fields">
        <div className="field"><span className="field-label">{t("当前视角")}</span><span>{t(state?.manualViewConfirmed?"已手动确认":"尚未确认")}</span></div>
        <Button variant="outline" disabled={busy||!state||!state.canConfirmView&&!state.manualViewConfirmed} onClick={()=>void confirm()}>
          {t(state?.manualViewConfirmed?"撤销视角确认":"确认本地可控视角")}
        </Button>
      </div>
    </FeatureSection>
    <MouseMovementAlgorithms choices={[mouseMovementDefinitions.smooth,mouseMovementDefinitions.direct]} defaultSelection="smooth">
      <MovementSpeedFields/>
    </MouseMovementAlgorithms>
  </>;
}
