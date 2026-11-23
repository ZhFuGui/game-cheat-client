import { z } from "zod";

export const valorantSettingsSchema = z.object({
  host: z.string().trim().min(1).max(255).refine(value => !value.includes("\0")),
  port: z.number().int().min(1).max(65535),
  hz: z.number().finite().min(1).max(300),
  valorantSkeletonHz: z.number().finite().min(1).max(300),
  valorantViewHz: z.number().finite().min(1).max(300),
  valorantShowSelf:z.boolean(),
  valorantShowAllies:z.boolean(),
  valorantShowEnemies:z.boolean(),
  valorantShowUnknown:z.boolean(),
  valorantShowCamera:z.boolean(),
  valorantMetersPerUnit:z.number().finite().min(0).max(1000),
  valorantWindowExecutable:z.string().min(5).max(240).regex(/^[^\\/:*?"<>|\x00-\x1f]+\.exe$/),
  dpi: z.number().int().min(100).max(32000),
  renderAdapter: z.string().min(1).max(255),
}).strict();
export type ValorantSettings = z.infer<typeof valorantSettingsSchema>;
