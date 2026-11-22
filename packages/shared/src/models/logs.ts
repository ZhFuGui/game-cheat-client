import { z } from "zod";
import { workspaceContextSchema } from "./workspaces";
export const logEntrySchema = z.object({
    id: z.string(),
    level: z.enum(["debug", "info", "warning", "error"]),
    time: z.string(),
    message: z.string(),
    workspace: workspaceContextSchema.optional(),
});
export type LogEntry = z.infer<typeof logEntrySchema>;
export const logBatchSchema = z.object({
    entries: z.array(logEntrySchema).max(5000),
    cursor: z.string(),
});
export type LogBatch = z.infer<typeof logBatchSchema>;
