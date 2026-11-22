import { z } from "zod";
export const portsSchema = z.array(z.object({ port: z.string(), label: z.string() }));
export const graphicsAdaptersSchema = z.array(z.object({
    value: z.string(),
    label: z.string(),
    detail: z.string().optional(),
    automatic: z.boolean().optional(),
    software: z.boolean().optional(),
}));
