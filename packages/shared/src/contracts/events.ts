import { z } from "zod";
import { workspaceSettingsSchema, speedPresetsSchema } from "../models/settings";
import { espSettingsSchema } from "../models/esp";
import { runtimeStateSchema } from "../models/runtime";
import { learningStateSchema } from "../models/learning";
import { mouseReportsStateSchema } from "../models/mouseReports";
import { portsSchema, graphicsAdaptersSchema } from "../models/devices";
import { algorithmSchema, algorithmSectionsSchema } from "../models/algorithm";
import { logBatchSchema } from "../models/logs";
import { preferencesSchema } from "../models/preferences";
import { workspaceCatalogSchema } from "../models/workspaces";
import { applicationSettingsSchema } from "../models/application";
import { deviceStateSchema } from "../models/deviceState";
export const eventSchemas = {
    workspace: workspaceCatalogSchema,
    application: applicationSettingsSchema,
    devices: deviceStateSchema,
    values: workspaceSettingsSchema,
    espValues: espSettingsSchema,
    speedPresets: speedPresetsSchema,
    state: runtimeStateSchema,
    learning: learningStateSchema,
    mouseReports: mouseReportsStateSchema,
    ports: portsSchema,
    graphicsAdapters: graphicsAdaptersSchema,
    algorithm: z.object({
        algorithm: algorithmSchema,
        algorithmSections: algorithmSectionsSchema,
    }),
    logs: logBatchSchema,
    preferences: preferencesSchema,
    bridge: z.object({
        connected: z.boolean(),
        session: z.string().optional(),
        message: z.string().optional(),
    }),
    error: z.object({ code: z.string(), message: z.string() }),
} as const;
export type Topic = keyof typeof eventSchemas;
export type EventPayload<K extends Topic> = z.output<(typeof eventSchemas)[K]>;
