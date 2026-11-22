import {z} from "zod";
export const sourceDataSchema=z.object({
  streams:z.array(z.object({id:z.string().min(1).max(32),title:z.string().max(128),available:z.boolean(),
    format:z.enum(["protobuf","materialized"])}).strict()).max(16),
  selected:z.string().max(32).nullable(),revision:z.string().max(512).nullable(),
  payload:z.string().max(3*1024*1024).nullable(),unchanged:z.boolean(),error:z.string().max(4096).nullable(),
}).strict();
export type SourceData= z.infer<typeof sourceDataSchema>;
