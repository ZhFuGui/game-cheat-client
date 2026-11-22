import { lazy } from "react";
import { cs2BestDamageExtensionId } from "@mousebox/shared";
import type { GameExtensionUi } from "~/games/contracts";
export const bestDamageExtensionUi: GameExtensionUi = {
  id: cs2BestDamageExtensionId,
  slots: {
    "esp-settings": lazy(() => import("./Settings").then(module => ({ default: module.BestDamageSettings }))),
  },
};
