import { z } from "zod";
export const applicationSettingsSchema = z.object({
  dpi: z.number().int().min(100).max(32000),
  renderAdapter: z.string().min(1).max(256),
}).strict();
export type ApplicationSettings = z.infer<typeof applicationSettingsSchema>;
