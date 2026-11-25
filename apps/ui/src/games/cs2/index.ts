import { lazy } from "react";
import { cs2SettingsSchema } from "@mousebox/shared";
import type { GameUiModule } from "../contracts";
import { coverExtensionUi } from "./extensions/cover";
import { remainingDamageExtensionUi } from "./extensions/remaining-damage";
import { headDamageExtensionUi } from "./extensions/head-damage";
import { bestDamageExtensionUi } from "./extensions/best-damage";

export const cs2UiModule: GameUiModule = {
  id: "cs2",
  navigationHelp:{"/triggerbot":{title:"检测与点击原理",paragraphs:[
    "起点使用已核验的射击视点，方向包含原生眼角基准与当前P/U后坐。使用资源Hitbox与本批完整骨姿态，不以屏幕骨点或枪口模型代替。",
    "每次左键按下所设时长后由固件自行抬起；同一数据帧最多触发一次。物理左键优先，断流、失效或关闭会话不会保持自动按下。",
    "可选择静态掩体判断。开启后可同时启用反应时间判定，按各目标首次露出骨点分别计时，完全遮挡后重新计时。",
  ]}},
  externalEsp: true,
  extensions: [coverExtensionUi, remainingDamageExtensionUi, headDamageExtensionUi, bestDamageExtensionUi],
  settingValidators: cs2SettingsSchema.shape,
  AimSettings: lazy(() => import("./AimSettings").then(module => ({ default: module.AimSettings }))),
  aimGeneral: [
    {id:"targets",component:lazy(()=>import("./AimGeneralSettings").then(module=>({default:module.Cs2TargetSettings})))},
    {id:"runtime",component:lazy(()=>import("./AimGeneralSettings").then(module=>({default:module.Cs2RuntimeSettings})))},
    {id:"solution",component:lazy(()=>import("./RecoilScheme").then(module=>({default:module.Cs2RecoilScheme})))},
    {id:"mapping",component:lazy(()=>import("./AimGeneralSettings").then(module=>({default:module.Cs2InputMapping})))},
    {id:"timing",component:lazy(()=>import("./AimGeneralSettings").then(module=>({default:module.Cs2TimingSettings})))},
  ],
  Triggerbot: lazy(() => import("./Triggerbot").then(module => ({ default: module.Triggerbot }))),
  Rage: lazy(() => import("./Rage").then(module => ({ default: module.Rage }))),
  SourcePanel: lazy(() => import("./ReaderConnection").then(module => ({ default: module.ReaderConnection }))),
  DataPanel: lazy(() => import("./DataPanel").then(module => ({ default: module.DataPanel }))),
  EspSettings: lazy(() => import("./EspSettings").then(module => ({ default: module.EspSettings }))),
  Radar: lazy(() => import("./radar/RadarPage")),
  settings: [
    {
      id: "cs2-source",
      title: "CS2 数据",
      description: "采样有效期与显式世界单位比例。",
      component: lazy(() => import("./DataSettings").then(module => ({ default: module.DataSettings }))),
    },
    {
      id: "aimbot-algorithm",
      title: "CS2 Aimbot 原理",
      description: "目标、时序、后坐求值与设备计数换算的实际来源。",
      capability: "algorithm-details",
      component: lazy(() => import("./DataPrinciples").then(module => ({ default: module.DataPrinciples }))),
    },
    {
      id: "aimbot-mouse",
      title: "Aimbot 鼠标移动",
      description: "设备计数、比例速度、学习模型与已发位移反馈。",
      capability: "algorithm-details",
      component: lazy(() => import("./DataPrinciples").then(module => ({ default: module.MousePrinciples }))),
    },
  ],
};
