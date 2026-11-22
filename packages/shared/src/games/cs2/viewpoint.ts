import { z } from "zod";

const vector = z.tuple([z.number().finite(), z.number().finite(), z.number().finite()]).nullable();
export const cs2ViewpointSchema = z.object({
  readOnly: z.literal(true),
  capture: z.string().regex(/^\d+$/),
  pawn: z.string().regex(/^\d+$/).nullable(),
  bodyMatched: z.boolean(),
  inputAngles: vector,
  pawnInputAngles: vector,
  readerEyePosition: vector,
  bodyPosition: vector,
  rawViewOffset: vector,
  bodyRotation: vector,
  bodyScale: z.number().finite().nullable(),
}).strict().superRefine((value, context) => {
  const bodyValues = [value.pawnInputAngles, value.bodyPosition, value.rawViewOffset, value.bodyRotation, value.bodyScale];
  if (value.bodyMatched ? value.pawn === null : bodyValues.some(item => item !== null))
    context.addIssue({ code: z.ZodIssueCode.custom, message: "Body viewpoint fields require the matched local pawn" });
});
export type Cs2Viewpoint = z.infer<typeof cs2ViewpointSchema>;
