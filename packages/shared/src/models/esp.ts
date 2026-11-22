import { z } from "zod";
export const espSettingsSchema = z
    .object({
    marker: z.enum(["corners", "point", "estimated"]),
    aspect: z.enum(["16:9", "16:10", "21:9", "4:3", "viewport"]),
    fovAxis: z.enum(["horizontal", "vertical"]),
    estimatedHeightMeters: z.number().finite().positive(),
    maximumDistanceMeters: z.number().finite().nonnegative(),
    names: z.boolean(),
    distance: z.boolean(),
    bones: z.boolean(),
    tracers: z.boolean(),
    crosshair: z.boolean(),
    aimRange: z.boolean(),
    targetHighlight: z.boolean(),
    teammates: z.boolean(),
    opponents: z.boolean(),
    bots: z.boolean(),
    self: z.boolean(),
})
    .strict();
export type EspSettings = z.infer<typeof espSettingsSchema>;
export type EspKey = keyof EspSettings;
