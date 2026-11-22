import { z } from "zod";
import { preferencesSchema } from "../models/preferences";
import { settingChange, espChange } from "./changes";
import { empty, ok, command } from "./common";
export const configurationContracts = {
    "settings.set": command(settingChange, ok),
    "settings.save": command(empty, ok),
    "esp.set": command(espChange, ok),
    "esp.reset": command(empty, ok),
    "preferences.set": command(preferencesSchema.partial().strict(), preferencesSchema),
    "migration.choose": command(z.object({ id: z.string().min(1) }).strict(), ok),
} as const;
