import {useTranslation} from "react-i18next";
import {Hint} from "~/components/shared/Content";
import {SettingChoice} from "~/components/shared/SettingField";
import {useRuntimeStore} from "~/runtime/state";
import {DeviceSpeedBasis} from "./DeviceSpeedBasis";
import {speedNames} from "./SpeedPresets";

/** The existing game-scoped speed setting; device limits still come from native negotiation. */
export function MovementSpeedFields({disabled=false,notice}:{disabled?:boolean;notice?:string}) {
  const {t}=useTranslation(),presets=useRuntimeStore(s=>s.speedPresets);
  return <>
    <SettingChoice setting="speed" label="跟随速度" disabled={disabled} options={presets.map(p=>({
      value:p.value,label:`${t(speedNames[p.value])} · ${p.percent}% · ${p.maximumCountsPerSecond} ${t("计数/秒")}`,
    }))} help={<p>{t("显示每轴鼠标计数速度上限，不是像素/秒；实际转向速度随灵敏度与 FOV 变化。")}</p>}/>
    <div className="aim-speed-notes">
      <DeviceSpeedBasis/>
      {notice&&<Hint>{notice}</Hint>}
    </div>
  </>;
}
