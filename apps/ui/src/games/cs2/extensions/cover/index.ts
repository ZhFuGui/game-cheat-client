import { lazy } from "react";
import { cs2CoverExtensionId } from "@mousebox/shared";
import type { GameExtensionUi } from "~/games/contracts";

export const coverExtensionUi: GameExtensionUi = {
  id: cs2CoverExtensionId,
  slots: {
    "esp-settings": lazy(() => import("./Settings").then(module => ({ default: module.CoverSettings }))),
  },
};
