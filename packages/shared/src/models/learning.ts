import { z } from "zod";
export const learningStateSchema = z
    .object({
    recording: z.boolean(),
    training: z.boolean(),
    active: z.boolean(),
    variation: z.boolean(),
    dpi: z.number(),
    sessions: z.number(),
    trials: z.number(),
    samples: z.number(),
    feedback: z.string(),
    model: z.string(),
    modelPath: z.string(),
    canApply: z.boolean(),
    status: z.string(),
    trace: z
        .array(z.object({
        x: z.number().finite(),
        y: z.number().finite(),
        t: z.number().finite().optional(),
    }))
        .max(4096)
        .optional(),
    captureX: z.number().finite().optional(),
    captureY: z.number().finite().optional(),
    replaying: z.boolean().optional(),
    ioBusy: z.boolean().optional(),
})
    .passthrough();
export type LearningState = z.infer<typeof learningStateSchema>;
