import {z} from "zod";
import {command} from "../../contracts/common";
import {mouseTrajectorySettingsSchema} from "../../models/mouseMovement";

const coefficient=z.number().finite().min(-10).max(10).refine(value=>value===0||Math.abs(value)>=1e-7);
export const valorantAimSettingsSchema=z.object({
  aimEnabled:z.boolean(),valorantAimEnemies:z.boolean(),valorantAimTeammates:z.boolean(),valorantCheckForeground:z.boolean(),
  valorantAimPart:z.number().int().min(0).max(3),valorantTargetPriority:z.number().int().min(0).max(1),
  valorantAimCone:z.number().finite().min(.1).max(89),valorantAimDistanceUnits:z.number().finite().min(0).max(1e9),
  valorantAimMaxAgeMs:z.number().int().min(10).max(1000),valorantResponseMs:z.number().int().min(0).max(250),
  valorantYawPerCount:coefficient,valorantPitchPerCount:coefficient,
  speed:z.number().int().min(0).max(4),trigger:z.union([z.literal(8),z.literal(16)]),
  mouseMovementAlgorithm:z.enum(["smooth","direct"]),
}).merge(mouseTrajectorySettingsSchema);

export const valorantAimStateSchema=z.object({
  enabled:z.boolean(),active:z.boolean(),block:z.string().max(128),basis:z.literal("manual-camera-feedback"),
  manualViewConfirmed:z.boolean(),canConfirmView:z.boolean(),heldTarget:z.string().regex(/^[1-9]\d{0,19}$/).nullable(),
  pending:z.number().int().nonnegative(),sampleAgeMs:z.number().finite().nonnegative().nullable(),
  distanceUnits:z.number().finite().nonnegative().nullable(),lifeStateKnown:z.boolean(),
  yawPerCount:z.number().finite().nullable(),pitchPerCount:z.number().finite().nullable(),
}).strict();
export const valorantAimContracts={
  "game.valorant.aim.confirmView":command(z.object({confirmed:z.boolean()}).strict(),z.object({confirmed:z.boolean()}).strict()),
} as const;
