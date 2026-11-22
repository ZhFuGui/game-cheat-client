import { z } from "zod";
export const empty = z.object({}).strict();
export const path = z.string().min(1).max(32768);
export const ok = z.object({ ok: z.literal(true) }).passthrough();
export const command = <I extends z.ZodTypeAny, O extends z.ZodTypeAny>(input: I, output: O) => ({ input, output });
