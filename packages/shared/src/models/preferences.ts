import { z } from "zod";
import { SIDEBAR_MIN_WIDTH, SIDEBAR_MAX_WIDTH } from "./sidebar";
const pageId = z.string().regex(/^[a-z][a-z0-9-]{0,63}$/);
const pageTabOrderSchema = z.record(pageId, z.array(pageId).max(32).refine(ids => new Set(ids).size === ids.length))
    .refine(groups => Object.keys(groups).length <= 64);
export const preferencesSchema = z
    .object({
    language: z.enum(["zh", "en"]),
    colorMode: z.enum(["light", "dark"]),
    themePreset: pageId.optional(),
    sidebarWidth: z.number().int().min(SIDEBAR_MIN_WIDTH).max(SIDEBAR_MAX_WIDTH).optional(),
    sidebarCollapsed: z.boolean().optional(),
    pageTabOrder: pageTabOrderSchema.optional(),
    espPlacement: z.enum(["embedded","overlay"]).optional(),
    espCapture: z.enum(["include","exclude"]).optional(),
    espRenderer: z.enum(["default","software"]).optional(),
})
    .strict();
export type UiPreferences = z.infer<typeof preferencesSchema>;
