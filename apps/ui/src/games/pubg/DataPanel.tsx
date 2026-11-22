import {useTranslation} from "react-i18next";
import {Section,display} from "~/components/shared/Content";
import {Button} from "~/components/ui/button";
import {useRuntimeStore} from "~/runtime/state";
import {useCommand} from "~/hooks/useCommand";

export function DataPanel(){
 const {t}=useTranslation(),{run}=useCommand();
 const state=useRuntimeStore(s=>s.state);
 const loadRecording=async()=>{
  const selection=await run("file.open",{purpose:"recording"});
  if(selection?.paths[0])await run("recording.load",{path:selection.paths[0]});
 };
 return <>
  <div className="dashboard-statistics">{[
   ["本批人物",state.characters],["当前目标",state.target],["通道间采样偏差 (ms)",state.gap],
  ].map(([label,value])=><div key={String(label)}><span>{t(String(label))}</span><strong>{display(value)}</strong></div>)}</div>
  <Section title="PUBG 当前数据"><dl className="details-grid">{[
   ["当前 FOV",state.fov],["目标距离",state.targetDistance],["目标角差",state.angle],
   ["当前物品",state.hasCurrentItem===false?t("空手"):state.hasCurrentItem===true?state.currentWeaponClass||t("当前物品名称不可用"):t("手持状态未知")],
   ["观战人数",state.spectators],
  ].map(([label,value])=><div key={String(label)}><dt>{t(String(label))}</dt><dd>{display(value)}</dd></div>)}</dl>
  <p className="section-footnote">{state.control||t("等待链路与当前数据")}</p></Section>
  <div className="actions"><Button variant="outline" onClick={()=>void loadRecording()}>{t("载入录制样本")}</Button></div>
 </>;
}
