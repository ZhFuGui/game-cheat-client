import {useTranslation} from "react-i18next";
import {Section,display} from "~/components/shared/Content";
import {useRuntimeStore} from "~/runtime/state";
import {RecoilDiagnostics} from "./RecoilDiagnostics";
import {ViewpointDiagnostics} from "./ViewpointDiagnostics";
import {AccuracyDiagnostics} from "./AccuracyDiagnostics";
export function DataPanel(){
 const {t}=useTranslation();const state=useRuntimeStore(s=>s.state);
 const rows=[
  ["当前地图",state.cs2Map?.name],["地图资源",state.cs2Map?.resourcePath],["地图代次",state.cs2Map?.generation],
  ["本批人物",state.characters],["CS2 武器数",state.cs2Weapons],["游戏 Build",state.cs2Build],["游戏进程",state.cs2ProcessId],
  ["本地血量",state.cs2Health],["当前物品",state.currentWeaponClass],["射击计数",state.cs2ShotsFired],["原始灵敏度",state.cs2Sensitivity],
  ["FOV 覆盖值",state.cs2FovOverride],["输入视角",state.cs2ViewAngles],["相机 view-punch 基角",state.cs2ViewPunch],
  ["可预测 AimPunch 基角",state.cs2PredictablePunch],["非预测 AimPunch 基角",state.cs2UnpredictablePunch],
  ["原生后坐序号",state.cs2RecoilIndex],["原生精度惩罚",state.cs2AccuracyPenalty],
 ];
 return <><AccuracyDiagnostics/><ViewpointDiagnostics/><Section title="CS2 当前数据" help={<> <p>{t("角度为 Reader 原始值，未施加补偿。FOV 覆盖值为 0 表示未覆盖；它不是最终 FOV。")}</p><p>{t("原生数据与纯求值在此查看；共享算法与输入设置位于 Aim 常规，选人和侧键设置位于 Aimbot。")}</p> </>}><dl className="details-grid">{rows.map(([label,value])=><div key={String(label)}><dt>{t(String(label))}</dt><dd>{Array.isArray(value)?value.map(n=>typeof n==="number"?n.toFixed(4):String(n)).join(" / "):display(value as string|number|null|undefined)}</dd></div>)}</dl>
 </Section>
 <RecoilDiagnostics value={state.cs2Recoil}/></>;
}
