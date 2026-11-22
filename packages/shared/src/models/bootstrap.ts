import { z } from "zod";
import { preferencesSchema } from "./preferences";
import { workspaceSettingsSchema, speedPresetsSchema, acceptsGameSettings } from "./settings";
import { espSettingsSchema } from "./esp";
import { runtimeStateSchema } from "./runtime";
import { learningStateSchema } from "./learning";
import { mouseReportsStateSchema } from "./mouseReports";
import { portsSchema, graphicsAdaptersSchema } from "./devices";
import { algorithmSchema, algorithmSectionsSchema } from "./algorithm";
import { workspaceCatalogSchema } from "./workspaces";
import { applicationSettingsSchema } from "./application";
import { deviceStateSchema } from "./deviceState";
const nativeSnapshotObject = z
    .object({
    values: workspaceSettingsSchema,
    speedPresets: speedPresetsSchema,
    espValues: espSettingsSchema,
    state: runtimeStateSchema,
    learning: learningStateSchema,
    mouseReports: mouseReportsStateSchema,
    ports: portsSchema,
    graphicsAdapters: graphicsAdaptersSchema,
    algorithm: algorithmSchema,
    algorithmSections: algorithmSectionsSchema,
    workspace: workspaceCatalogSchema.optional(),
    application: applicationSettingsSchema.optional(),
    devices: deviceStateSchema.optional(),
})
    .passthrough();
const verifyGame=(value:{values:unknown;workspace?:z.infer<typeof workspaceCatalogSchema>},context:z.RefinementCtx)=>{
 if(value.workspace?.active&&!acceptsGameSettings(value.workspace.active.gameId,value.values))
  context.addIssue({code:"custom",path:["values"],message:"Game settings do not match workspace"});
};
export const nativeSnapshotSchema=nativeSnapshotObject.superRefine(verifyGame);
export type NativeSnapshot = z.infer<typeof nativeSnapshotSchema>;
export const bootstrapSchema = nativeSnapshotObject.extend({
    preferences: preferencesSchema,
    version: z.string(),
    session: z.string(),
    migration: z
        .object({
        required: z.boolean(),
        sources: z.array(z.object({
            id: z.string(),
            path: z.string(),
            modifiedAt: z.string().optional(),
        })),
    })
        .optional(),
}).superRefine(verifyGame);
export type Bootstrap = z.infer<typeof bootstrapSchema>;
