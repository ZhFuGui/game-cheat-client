import { z } from "zod";
export const surfaceSchema = z
    .object({
    kind: z.enum(["esp", "learning", "reports"]),
    x: z.number().finite(),
    y: z.number().finite(),
    width: z.number().finite().min(0).max(32768),
    height: z.number().finite().min(0).max(32768),
    scale: z.number().finite().positive().max(16),
    visible: z.boolean(),
    active: z.boolean().optional(),
    occluded: z.boolean().optional(),
})
    .strict();
import { ok, command } from "./common";
export const renderingContracts = {
    "surface.update": command(surfaceSchema, ok),
    "surface.palette": command(z
        .object({
        canvas: z.string().regex(/^#[0-9a-fA-F]{6}$/),
        grid: z.string().regex(/^#[0-9a-fA-F]{6}$/),
        guide: z.string().regex(/^#[0-9a-fA-F]{6}$/),
        text: z.string().regex(/^#[0-9a-fA-F]{6}$/),
        entity: z.string().regex(/^#[0-9a-fA-F]{6}$/),
        selected: z.string().regex(/^#[0-9a-fA-F]{6}$/),
        language: z.enum(["zh", "en"]),
    })
        .strict(), ok),
} as const;
