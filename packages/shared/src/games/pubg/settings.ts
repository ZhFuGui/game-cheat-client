import { z } from "zod";
import {mouseTrajectorySettingsSchema} from "../../models/mouseMovement";
export const pubgSettingsSchema = z.object({
    aimEnabled: z.boolean().default(true),
    aimVersion: z.literal("v1").default("v1"),
    mouseMovementAlgorithm: z.literal("smooth").default("smooth"),
    host: z.string().trim().min(1).max(255),
    port: z.number().int().min(1).max(65535),
    hz: z.number().int().min(1).max(240),
    general: z.number().int().min(0).max(100),
    aim: z.number().int().min(0).max(100),
    ads: z.number().int().min(0).max(100),
    vertical: z.number().finite().min(0.1).max(5),
    inverted: z.boolean(),
    skew: z.number().int().min(0).max(5000),
    livingLocalSkew: z.number().int().min(0).max(5000),
    mapping: z.number().int().min(0).max(1),
    mode: z.number().int().min(-1).max(3),
    measurement: z.number().int().min(0).max(3),
    speed: z.number().int().min(0).max(4),
    part: z.number().int().min(0).max(4),
    partSR: z.number().int().min(-1).max(4),
    partDMR: z.number().int().min(-1).max(4),
    partAR: z.number().int().min(-1).max(4),
    partSMG: z.number().int().min(-1).max(4),
    partLMG: z.number().int().min(-1).max(4),
    partShotgun: z.number().int().min(-1).max(4),
    partHandgun: z.number().int().min(-1).max(4),
    partOther: z.number().int().min(-1).max(4),
    targetPriority: z.union([z.literal(0), z.literal(1)]),
    distance: z.number().finite().min(1).max(5000),
    zeroing: z.union([
        z.literal(100),
        z.literal(200),
        z.literal(300),
        z.literal(400),
        z.literal(500),
    ]),
    gravity: z.number().finite().min(0).max(100),
    trigger: z.union([z.literal(8), z.literal(16)]),
})
    .merge(mouseTrajectorySettingsSchema).strict();
export type PubgSettings = z.infer<typeof pubgSettingsSchema>;

