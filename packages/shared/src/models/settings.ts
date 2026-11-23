import { z } from "zod";
import { applicationSettingsSchema } from "./application";
import { pubgSettingsSchema } from "../games/pubg/settings";
import { cs2SettingsSchema } from "../games/cs2/settings";
import { valorantSettingsSchema, type ValorantSettings } from "../games/valorant/settings";
import {mouseMovementAlgorithmSchema} from "./mouseMovement";
/** Compatibility projection for existing commands; ownership is separated in storage and UI. */
export const settingsSchema = applicationSettingsSchema.merge(pubgSettingsSchema).strict();
export type Settings = z.infer<typeof settingsSchema>;
export const settingValidators = {...settingsSchema.shape,...cs2SettingsSchema.omit({host:true,port:true,hz:true,dpi:true,renderAdapter:true}).shape,
    ...valorantSettingsSchema.omit({host:true,port:true,hz:true,dpi:true,renderAdapter:true}).shape,
    hz: z.union([settingsSchema.shape.hz, cs2SettingsSchema.shape.hz]),
    mouseMovementAlgorithm:mouseMovementAlgorithmSchema,
    trigger:z.union([settingsSchema.shape.trigger,cs2SettingsSchema.shape.trigger,valorantSettingsSchema.shape.trigger])};
export type SettingKey = keyof typeof settingValidators;
type CommonSettings=Pick<Settings,"host"|"port"|"hz"|"dpi"|"renderAdapter">;
export type WorkspaceSettings=CommonSettings & {aimVersion?:import("../games/cs2/aimModes").Cs2AimModeId;mouseMovementAlgorithm?:import("./mouseMovement").MouseMovementAlgorithm} & Partial<Omit<Settings,keyof CommonSettings|"aimVersion"|"mouseMovementAlgorithm">> & Partial<Omit<import("../games/cs2/settings").Cs2Settings,keyof CommonSettings|"aimVersion"|"mouseMovementAlgorithm">> & Partial<Omit<ValorantSettings,keyof CommonSettings|"mouseMovementAlgorithm"|"trigger">>;
export const workspaceSettingsSchema=z.union([settingsSchema,cs2SettingsSchema,valorantSettingsSchema]).transform(value=>value as WorkspaceSettings);
export function acceptsGameSettings(gameId:string|undefined,value:unknown):boolean {
 return gameId==="valorant"?valorantSettingsSchema.safeParse(value).success:gameId==="cs2"?cs2SettingsSchema.safeParse(value).success:gameId==="pubg"?settingsSchema.safeParse(value).success:false;
}
export const speedPresetsSchema = z.array(z.object({
    value: z.number().int().min(0).max(4),
    maximumCountsPerSecond: z.number().finite().positive(),
    acceleration: z.number().finite().positive(),
    maximumStepCounts: z.number().int().positive(),
    percent: z.number().finite().positive().max(100),
    intervalMs: z.number().int().positive(),
})).length(5);


