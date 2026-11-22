import { z } from "zod";

export const readerStageAverageSchema=z.object({
  averageMs:z.number().finite().nonnegative().nullable(),
  uncertaintyMs:z.number().finite().nonnegative().nullable(),
  samples:z.number().int().nonnegative(),
  status:z.enum(["ready","waiting-data","missing-timing","waiting-clock","invalid-timing","no-samples"]),
}).strict();
export const readerChainSchema=z.object({
  windowMs:z.literal(2000),
  read:readerStageAverageSchema,
  prepare:readerStageAverageSchema,
  transport:readerStageAverageSchema,
}).strict();
export type ReaderStageAverage=z.infer<typeof readerStageAverageSchema>;

export const readerMetricsSchema = z.object({
  windowMs: z.literal(1000),
  uploadBytesPerSecond: z.number().finite().nonnegative(),
  downloadBytesPerSecond: z.number().finite().nonnegative(),
  gpuCompletionAvailable: z.boolean(),
  channels: z.array(z.object({
    category: z.number().int().positive(),
    connection: z.string().regex(/^\d+$/),
    name: z.string().min(1).max(64),
    status: z.enum(["ready", "waiting-data", "waiting-clock", "waiting-draw"]),
    averageMs: z.number().finite().nonnegative().nullable(),
    uncertaintyMs: z.number().finite().nonnegative().nullable(),
    samples: z.number().int().nonnegative(),
    receivedFramesPerSecond: z.number().int().nonnegative(),
    chain:readerChainSchema.optional(),
  }).strict()).max(16),
}).strict();

export type ReaderMetrics = z.infer<typeof readerMetricsSchema>;
