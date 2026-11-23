import { lazy } from "react";
import { valorantSettingsSchema } from "@mousebox/shared";
import type { GameUiModule } from "../contracts";

export const valorantUiModule: GameUiModule = {
  id: "valorant",
  settingValidators: valorantSettingsSchema.shape,
  SourcePanel: lazy(() => import("./ReaderConnection").then(module => ({default: module.ReaderConnection}))),
  DataPanel: lazy(() => import("./DataPanel").then(module => ({default: module.DataPanel}))),
  Radar: lazy(() => import("./radar/RadarPage")),
  settings: [],
};
