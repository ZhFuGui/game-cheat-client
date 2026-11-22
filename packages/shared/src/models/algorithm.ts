import { z } from "zod";
export const algorithmFactSchema = z.object({
    id: z.string(),
    title: z.string(),
    current: z.string(),
    source: z.string(),
    principle: z.string(),
    status: z.string(),
    phase: z.string(),
});
export type AlgorithmFact = z.infer<typeof algorithmFactSchema>;
export const algorithmSchema = z
    .object({
    summary: z.string().optional(),
    mouseSummary: z.string().optional(),
    assumptionSummary: z.string().optional(),
    note: z.string().optional(),
    revision: z.string().optional(),
    facts: z.record(algorithmFactSchema).optional(),
})
    .passthrough();
export type Algorithm = z.infer<typeof algorithmSchema>;
export const algorithmSectionsSchema = z.array(z.object({ title: z.string(), ids: z.array(z.string()), scope: z.enum(["aimbot","mouse"]).optional() }));
