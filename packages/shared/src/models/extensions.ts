import { z } from "zod";

export const extensionIdSchema = z.string().regex(/^[a-z][a-z0-9-]*\.[a-z][a-z0-9-]*$/).max(96);
export const gameExtensionStateSchema = z.object({
  id: extensionIdSchema,
  phase: z.enum(["disabled", "waiting", "loading", "ready", "degraded", "error", "paused"]),
  code: z.string().max(96),
  message: z.string().max(1024),
  enabled: z.boolean(),
  capture: z.string().regex(/^\d+$/),
  queries: z.number().int().nonnegative(),
  visible: z.number().int().nonnegative(),
  occluded: z.number().int().nonnegative(),
  unknown: z.number().int().nonnegative(),
}).strict();
export const gameExtensionStatesSchema = z.array(gameExtensionStateSchema).max(32);
export type GameExtensionState = z.infer<typeof gameExtensionStateSchema>;
