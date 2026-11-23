import { lazy } from "react";
import { valorantSettingsSchema } from "@mousebox/shared";
import type { GameUiModule } from "../contracts";

export const valorantUiModule: GameUiModule = {
  id: "valorant",
  settingValidators: valorantSettingsSchema.shape,
  AimSettings:lazy(()=>import("./AimSettings").then(module=>({default:module.AimSettings}))),
  aimGeneral:[
    {id:"targets",component:lazy(()=>import("./AimGeneralSettings").then(module=>({default:module.TargetSettings})))},
    {id:"runtime",component:lazy(()=>import("./AimGeneralSettings").then(module=>({default:module.RuntimeSettings})))},
    {id:"mapping",component:lazy(()=>import("./AimGeneralSettings").then(module=>({default:module.InputMapping})))},
  ],
  SourcePanel: lazy(() => import("./ReaderConnection").then(module => ({default: module.ReaderConnection}))),
  DataPanel: lazy(() => import("./DataPanel").then(module => ({default: module.DataPanel}))),
  Radar: lazy(() => import("./radar/RadarPage")),
  EspSettings:lazy(()=>import("./EspSettings").then(module=>({default:module.EspSettings}))),
  VisualsSettings:lazy(()=>import("./VisualsSettings")),
  externalEsp:true,
  settings:[{id:"valorant-display",title:"窗口与单位",description:"",component:lazy(()=>import("./DisplayMapping"))}],
};
