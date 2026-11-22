import {z} from "zod";
import {command} from "../../contracts/common";
import {cs2MapSchema} from "./map";

const revision = z.string().regex(/^(0|[1-9]\d{0,19})$/);
const coordinate = z.number().finite();
const stat = z.number().int().nullable();
const weapon = z.object({name: z.string().max(4096), clip: stat, reserve: stat}).strict();

export const cs2RadarPlayerSchema = z.object({
  controller: z.number().int().min(1).max(0xffffffff),
  name: z.string().max(4096),
  team: z.union([z.literal(2), z.literal(3)]),
  local: z.boolean(),
  alive: z.boolean().nullable(),
  position: z.tuple([coordinate, coordinate, coordinate]).nullable(),
  yaw: coordinate.nullable(),
  health: stat,
  maxHealth: stat,
  armor: stat,
  weapon: weapon.nullable(),
}).strict();

export const cs2RadarFrameSchema = z.object({
  revision,
  scope: z.string().min(1).max(64),
  map: cs2MapSchema.nullable(),
  players: z.array(cs2RadarPlayerSchema).max(4096),
}).strict();

export const cs2RadarReplySchema = z.object({
  frame: cs2RadarFrameSchema.nullable(),
  unchanged: z.boolean(),
}).strict().refine(reply => !reply.unchanged || reply.frame === null);

export type Cs2RadarPlayer = z.infer<typeof cs2RadarPlayerSchema>;
export type Cs2RadarFrame = z.infer<typeof cs2RadarFrameSchema>;

export const cs2RadarContracts = {
  "game.cs2.radar.read": command(z.object({after: revision.optional()}).strict(), cs2RadarReplySchema),
} as const;
