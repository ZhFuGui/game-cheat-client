import {z} from "zod";
import {command} from "../../contracts/common";
import {valorantMapIdentitySchema,valorantLocalCharacterSchema} from "./source";

const id=z.string().regex(/^[1-9]\d{0,19}$/);
const vector=z.tuple([z.number().finite(),z.number().finite(),z.number().finite()]);
export const valorantRadarFrameSchema=z.object({
  revision:z.string().regex(/^\d{1,20}$/),
  scope:z.string().min(1).max(96),
  map:valorantMapIdentitySchema.nullable(),
  local:valorantLocalCharacterSchema,
  players:z.array(z.object({
    id,position:vector.nullable(),
    team:z.object({scheme:z.number().int().nonnegative().max(0xffffffff),key:z.string().max(128)}).strict().nullable(),
    relation:z.enum(["unknown","self","ally","opponent"]),
  }).strict()).max(4096),
  camera:vector.nullable(),
}).strict();
export type ValorantRadarFrame=z.infer<typeof valorantRadarFrameSchema>;
export type ValorantRadarPlayer=ValorantRadarFrame["players"][number];
export const valorantRadarContracts={
  "game.valorant.radar.read":command(z.object({after:z.string().regex(/^\d{1,20}$/).optional()}).strict(),
    z.object({unchanged:z.boolean(),frame:valorantRadarFrameSchema.nullable()}).strict()
      .refine(value=>!value.unchanged||value.frame===null)),
} as const;
