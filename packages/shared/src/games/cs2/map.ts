import { z } from "zod";

export const cs2MapSchema = z.object({
  status: z.number().int().min(0).max(5), generation: z.string().regex(/^(0|[1-9]\d*)$/).max(20), available: z.boolean(),
  name: z.string().min(1).max(4096).nullable(), resourcePath: z.string().min(1).max(4096).nullable(),
}).strict().superRefine((map, context) => {
  const valid = map.status === 1 && map.generation !== "0" && map.name !== null && map.resourcePath !== null;
  if (map.status === 1 && !valid || valid !== map.available || !map.available && (map.name !== null || map.resourcePath !== null))
    context.addIssue({ code: "custom", message: "Map presence contradicts availability" });
});
