import { lazy } from "react";
import { cs2RemainingDamageExtensionId } from "@mousebox/shared";
import type { GameExtensionUi } from "~/games/contracts";
export const remainingDamageExtensionUi: GameExtensionUi = {
  id: cs2RemainingDamageExtensionId,
  slots: {
    "esp-settings": lazy(() => import("./Settings").then(module => ({ default: module.RemainingDamageSettings }))),
  },
};
