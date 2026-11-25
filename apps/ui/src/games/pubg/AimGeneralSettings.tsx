import {useTranslation} from "react-i18next";
import {useShallow} from "zustand/react/shallow";
import {Hint} from "~/components/shared/Content";
import {FeatureSection} from "~/components/shared/FeatureSection";
import {SettingField} from "~/components/shared/SettingField";
import {useRuntimeStore} from "~/runtime/state";

export function PubgBallisticSettings() {
  const {t}=useTranslation();
  const state=useRuntimeStore(useShallow(({state})=>({calibrating:state.calibrating,
    aimReference:state.aimReference,nativeZeroingMeters:state.nativeZeroingMeters})));
  return <FeatureSection title="弹道计算"><div className="aim-fields">
    <SettingField setting="zeroing" label="相机模式归零距离" suffix="m" slider={{min:100,max:500,step:100}}
      disabled={state.calibrating||state.aimReference==="native"}
      help={<p>{t("相机参考使用手动等效归零；原生参考使用 Reader 中已含归零的方向，不再扣此角度。")}</p>}/>
    <SettingField setting="gravity" label="重力加速度" suffix="m/s²" slider={{min:0,max:100,step:0.1}} disabled={state.calibrating}
      help={<p>{t("0–100 m/s²，竖直向下；默认9.8，设为0可关闭重力下坠。所有枪械共用，自动保存。")}</p>}/>
    {state.aimReference==="native"&&<Hint>{t("当前使用原生方向，原生归零距离")}：{state.nativeZeroingMeters??"—"} m</Hint>}
  </div></FeatureSection>;
}

export function PubgTimingSettings() {
  const {t}=useTranslation();
  return <FeatureSection title="采样时序"><div className="aim-fields">
    <SettingField setting="skew" label="允许采样时间差" suffix="ms" slider={{min:0,max:5000,step:10}}
      help={<p>{t("用于Living与Local整帧的采样区间配对。")}</p>}/>
    <SettingField setting="livingLocalSkew" label="Living / Local 补偿时间差上限" suffix="ms" slider={{min:0,max:5000,step:10}}
      help={<p>{t("Local相机与Living己方身体速度的采样差上限；0–5000 ms，默认100 ms。自动保存，修改后松开侧键再按。")}</p>}/>
  </div></FeatureSection>;
}
