import {z} from "zod";

export const cs2HitboxStateSchema=z.object({
  phase:z.enum(["disabled","waiting-data","unsupported","waiting-pose","partial","ready"]),
  models:z.number().int().nonnegative(),hitboxes:z.number().int().nonnegative(),
  missing:z.number().int().nonnegative(),invalid:z.number().int().nonnegative(),
}).strict();
