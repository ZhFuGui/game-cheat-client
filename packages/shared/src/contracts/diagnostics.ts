import { z } from "zod";
import { logBatchSchema } from "../models/logs";
import { algorithmSchema, algorithmSectionsSchema } from "../models/algorithm";
import { empty, path, ok, command } from "./common";
import {sourceDataSchema} from "../models/sourceData";
export const diagnosticsContracts = {
    "reader.inspect":command(z.object({stream:z.string().max(32).optional(),after:z.string().max(512).optional()}).strict(),sourceDataSchema),
    "logs.read": command(z
        .object({ after: z.string(), limit: z.number().int().min(1).max(5000) })
        .strict(), logBatchSchema),
    "logs.clear": command(empty, ok),
    "logs.export": command(z.object({ path }).strict(), ok),
    "algorithm.subscribe": command(z.object({ enabled: z.boolean() }).strict(), ok),
    "algorithm.read": command(empty, z.object({
        algorithm: algorithmSchema,
        algorithmSections: algorithmSectionsSchema,
    })),
    "algorithm.copy": command(empty, ok),
    "ballistics.reload": command(empty, ok),
} as const;
