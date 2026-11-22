import { z } from "zod";
import { command, empty, ok } from "../../contracts/common";
import {cs2AimModeSchema} from "./aimModes";

const ratio = z.number().finite().refine(n => Math.abs(n) >= .05 && Math.abs(n) <= 20);
const derivative = z.number().finite().refine(n => Math.abs(n) >= 1e-7);
export const cs2CountCalibrationSchema = z.object({
  version: z.literal(1), build: z.number().int().min(0).max(0xFFFFFFFF), x: ratio, y: ratio,
  yawReference: derivative, pitchReference: derivative,
  responseUpperMs: z.number().finite().min(0).max(500),
}).strict();
const angle = z.tuple([z.number().finite(), z.number().finite(), z.number().finite()]).nullable();
export const cs2ControllerSchema = z.object({
  block: z.string().max(128), active: z.boolean(), down: z.boolean(), releaseRequired: z.boolean(),
  version:z.enum(["v1","v2","v3","v4"]).optional(), motionStatus:z.enum(["v1","branch-baseline","recoil-disabled","time-rebased","fast-recoil","tracking","output-rejected","direct"]).optional(),
  directionReference:z.enum(["native-basis","global-input"]).optional(),
  modeId:cs2AimModeSchema.optional(),comparisonFactor:z.string().max(64).optional(),
  movementAlgorithm:z.enum(["smooth","recoil-tracking","direct"]).optional(),
  calibrated: z.boolean(), model: z.boolean(),
  recoilOnly:z.boolean().optional(),
  variation:z.boolean().optional(),
  inputMapping:z.enum(["native","measured"]).optional(),inputMappingReady:z.boolean().optional(),
  countScaleX:z.number().finite().nullable().optional(),countScaleY:z.number().finite().nullable().optional(),
  targetController: z.string().nullable(), targetPawn: z.string().nullable(), targetName: z.string(),
  distanceUnits: z.number().finite().nonnegative().nullable(),
  sampleAgeMs: z.number().finite().nonnegative().nullable(), uncertaintyMs: z.number().finite().nonnegative().nullable(),
  horizonMs: z.number().finite().nonnegative().nullable(),
  recoilHorizonMs:z.number().finite().nonnegative().nullable().optional(),
  desired: angle, direction: angle, error: angle,
  yawPerCount: z.number().finite().nullable(), pitchPerCount: z.number().finite().nullable(),
  pendingCommands: z.number().int().min(0).max(512),
  heldTargetController:z.string().nullable().optional(), heldTargetPawn:z.string().nullable().optional(),
  heldTargetName:z.string().optional(), suspended:z.boolean().optional(),
  lastPause:z.string().max(128).optional(), lastPauseAgeMs:z.number().finite().nonnegative().nullable().optional(),
  pauseCount:z.string().regex(/^\d+$/).optional(), releaseReason:z.string().max(128).optional(),
  feedbackReference:z.enum(["unavailable","pawn-prediction-input","pawn-input","global-input"]).optional(),
  calibrating: z.boolean(), calibrationStatus: z.string().max(128), calibrationLeg: z.number().int().min(0).max(4),
  calibrationBlock:z.string().max(128).optional(),
  calibrationSampleAgeMs:z.number().finite().nonnegative().nullable().optional(),
  calibrationUncertaintyMs:z.number().finite().nonnegative().nullable().optional(),
  calibrationResult: cs2CountCalibrationSchema.nullable(),
}).strict().superRefine((state,ctx)=>{
  if (state.inputMapping==="native" && state.calibrated)
    ctx.addIssue({code:z.ZodIssueCode.custom,path:["calibrated"],message:"Native formula must not claim device measurement"});
  if (state.inputMapping==="measured" && state.inputMappingReady===true && !state.calibrated)
    ctx.addIssue({code:z.ZodIssueCode.custom,path:["inputMappingReady"],message:"Measured mapping requires an adopted record"});
});
export const cs2ControllerContracts = {
  "game.cs2.calibration.start": command(empty, ok),
  "game.cs2.calibration.cancel": command(empty, ok),
} as const;
export type Cs2ControllerState = z.infer<typeof cs2ControllerSchema>;
