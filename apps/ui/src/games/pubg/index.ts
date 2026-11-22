import { lazy } from "react";
import { pubgSettingsSchema } from "@mousebox/shared";
import type { GameUiModule } from "../contracts";

const GameInput = lazy(() => import("./GameInput").then(module => ({ default: module.GameInput })));
const Calibration = lazy(() => import("./Calibration").then(module => ({ default: module.Calibration })));
const AimbotAlgorithm = lazy(() => import("./AimbotPrinciples").then(module => ({ default: module.AimbotAlgorithm })));
const AimbotMouseMovement = lazy(() => import("./AimbotPrinciples").then(module => ({ default: module.AimbotMouseMovement })));

export const pubgUiModule: GameUiModule = {
  id: "pubg",
  settingValidators: pubgSettingsSchema.shape,
  AimSettings: lazy(() => import("./FollowSettings").then(module => ({ default: module.FollowSettings }))),
  aimGeneral: [
    {id:"solution",component:lazy(()=>import("./RecoilScheme").then(module=>({default:module.PubgRecoilScheme})))},
    {id:"movement",component:lazy(()=>import("./MouseMovementAlgorithms").then(module=>({default:module.PubgMouseMovementAlgorithms})))},
    {id:"ballistics",component:lazy(()=>import("./AimGeneralSettings").then(module=>({default:module.PubgBallisticSettings})))},
    {id:"timing",component:lazy(()=>import("./AimGeneralSettings").then(module=>({default:module.PubgTimingSettings})))},
  ],
  SourcePanel: lazy(() => import("./ReaderConnection").then(module => ({ default: module.ReaderConnection }))),
  DataPanel: lazy(() => import("./DataPanel").then(module => ({ default: module.DataPanel }))),
  EspSettings: lazy(() => import("./EspSettings").then(module => ({ default: module.EspSettings }))),
  settings: [
    { id: "input", title: "游戏输入", description: "PUBG 灵敏度、视角与输入换算。", component: GameInput },
    { id: "calibration", title: "校准", description: "此游戏的实测 FOV 与灵敏度记录。", capability: "input-calibration", component: Calibration },
    { id: "aimbot-algorithm", title: "Aimbot 算法", description: "此游戏的目标、弹道、参考方向与数据来源。", capability: "algorithm-details", component: AimbotAlgorithm },
    { id: "aimbot-mouse", title: "Aimbot 鼠标移动", description: "输入换算、设备比例速度与鼠标移动原理。", capability: "algorithm-details", component: AimbotMouseMovement },
  ],
};

