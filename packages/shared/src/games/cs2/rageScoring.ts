import {z} from "zod";
const score=z.number().finite().min(0).max(100);
export const rageScoringSchema=z.object({
  model:z.enum(["legacy","threat-v2"]),aim:score.nullable(),
  incomingDamage:z.number().finite().nonnegative().nullable(),incomingDamageScore:score.nullable(),
  outgoingDamageScore:score,threatScore:score.nullable(),lowAmmoDeduction:score,reloadDeduction:score,totalScore:score,
  degraded:z.boolean(),stabilized:z.boolean(),weapon:z.number().int().nonnegative().nullable(),
  clip:z.number().int().nonnegative().nullable(),reloading:z.boolean().nullable(),assumedFriendlyFire:z.boolean(),
}).strict();
