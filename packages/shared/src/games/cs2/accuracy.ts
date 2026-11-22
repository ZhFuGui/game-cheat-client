import { z } from "zod";

const measurement = z.object({
  status: z.enum(["valid", "unavailable", "unverified", "inconsistent"]),
  value: z.number().finite().nullable(),
  reason: z.string().max(256),
}).strict().superRefine((value, context) => {
  if ((value.status === "valid") !== (value.value !== null))
    context.addIssue({ code: z.ZodIssueCode.custom, message: "Only a valid accuracy measurement carries a value" });
});

// Presentation only. These amplitudes are neither angles nor firing permission.
export const cs2AccuracySchema = z.object({
  readOnly: z.literal(true),
  capture: z.string().regex(/^[1-9]\d*$/),
  weapon: z.string().regex(/^\d+$/).nullable(),
  owner: z.string().regex(/^\d+$/).nullable(),
  modelId: z.string().max(128).nullable(),
  branch: z.enum(["normal", "forced", "no-spread", "no-owner", "unavailable"]),
  inaccuracy: measurement,
  spread: measurement,
  components: z.object({
    penalty: z.number().finite().nullable(),
    movement: z.number().finite().nullable(),
    airborne: z.number().finite().nullable(),
    turning: z.number().finite().nullable(),
    lateral: z.number().finite().nullable(),
  }).strict().nullable(),
  readIntervalMs: z.number().finite().nonnegative().nullable(),
}).strict().superRefine((value, context) => {
  if ((value.inaccuracy.status === "valid" || value.spread.status === "valid") &&
      (value.weapon === null || value.modelId === null))
    context.addIssue({ code: z.ZodIssueCode.custom, message: "Accuracy values require their weapon and verified model" });
  if (value.inaccuracy.status === "valid" && value.branch === "unavailable")
    context.addIssue({ code: z.ZodIssueCode.custom, message: "A valid inaccuracy value requires its native branch" });
  if (value.components !== null && (value.inaccuracy.status !== "valid" || value.branch !== "normal"))
    context.addIssue({ code: z.ZodIssueCode.custom, message: "Only a valid normal branch carries accuracy components" });
});
export type Cs2Accuracy = z.infer<typeof cs2AccuracySchema>;
