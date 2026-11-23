import { z } from "zod";
import { command } from "../../contracts/common";

const identity = z.string().regex(/^[1-9]\d{0,19}$/);
const revision = z.string().regex(/^\d{1,20}$/);
const vector = z.tuple([z.number().finite(), z.number().finite(), z.number().finite()]);
export const valorantMapIdentitySchema=z.object({packagePath:z.string().max(4096),worldName:z.string().max(1024)}).strict();
export const valorantLocalCharacterSchema=z.object({state:z.enum(["unknown","absent","unresolved","resolved"]),entityId:identity.nullable()}).strict()
  .refine(value=>(value.state==="resolved")===(value.entityId!==null));
export const valorantCameraSchema = z.object({
  position: vector.nullable(), rotation: vector.nullable(), projection: z.number().int().min(0).max(2),
  fov: z.number().finite().nullable(), aspect: z.number().finite().nullable(),
  constrainAspect: z.boolean().nullable(), orthoWidth: z.number().finite().nullable(), engineTime: z.number().finite().nullable(),
}).strict();
export const valorantSourceSchema = z.object({
  context:z.object({map:valorantMapIdentitySchema.nullable(),local:valorantLocalCharacterSchema,
    referenceTeam:z.string().regex(/^\d{1,3}$/).nullable()}).strict().nullable().optional(),
  streams: z.array(z.object({
  stream: z.number().int().min(1).max(3), connected: z.boolean(), connecting: z.boolean(), current: z.boolean(),
  requestedHz: z.number().finite().min(1).max(300), subscribedHz: z.number().finite().min(1).max(300).nullable(),
  pending: z.boolean(), error: z.string(), subscriptionError: z.string(), reader: z.string(),
  epoch: identity.nullable(), capture: identity.nullable(), receivedAtMs: z.number().finite().nonnegative().nullable(),
  entities: z.number().int().nonnegative().nullable(), camera: valorantCameraSchema.nullable(),
}).strict()).length(3)}).strict();
const entity = z.union([
  z.object({id: identity, position: vector.nullable()}).strict(),
  z.object({id: identity, joints: z.array(vector.nullable()).length(22).nullable()}).strict(),
]);
export const valorantSceneSchema = z.object({
  revision,
  streams: z.array(z.object({reader: z.string().regex(/^[a-f0-9]{32}$/), epoch: identity, capture: identity,
    receivedAtMs: z.number().finite().nonnegative(), entities: z.array(entity).max(4096),
    camera: valorantCameraSchema.nullable()}).strict().nullable()).length(3),
}).strict();
export type ValorantScene = z.infer<typeof valorantSceneSchema>;
export const valorantSourceContracts = {
  "game.valorant.scene.read": command(z.object({after: revision.optional()}).strict(),
    z.object({unchanged: z.boolean(), frame: valorantSceneSchema.nullable()}).strict()
      .refine(value => !value.unchanged || value.frame === null)),
} as const;
