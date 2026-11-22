import { lazy } from "react";
import { cs2HeadDamageExtensionId } from "@mousebox/shared";
import type { GameExtensionUi } from "~/games/contracts";
export const headDamageExtensionUi: GameExtensionUi = {
  id: cs2HeadDamageExtensionId,
  slots: {
    "esp-settings": lazy(() => import("./Settings").then(module => ({ default: module.HeadDamageSettings }))),
  },
};
