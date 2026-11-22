import { z } from "zod";

const value = z.number().finite().nullable();
const angle = z.tuple([z.number().finite(), z.number().finite(), z.number().finite()]).nullable();
const gameTime = z.object({ tick: z.number().int().min(-2147483648).max(2147483647).nullable(), fraction: value }).strict().nullable();
const context = z.number().int().min(-2147483648).max(2147483647).nullable();
const eyeBasis = z.object({
  value: angle, source: z.number().int().min(0).max(7), nativeContext: context, interpolationParameter: value,
  resolvedSource:z.number().int().min(1).max(7).refine(n=>n!==6).nullable().optional(),
  sourceEntity:z.string().regex(/^\d+$/).nullable().optional(),
  delegationDepth:z.number().int().min(1).max(4294967295).nullable().optional(),
}).strict().superRefine((basis,ctx)=>{
  const any=basis.resolvedSource!=null||basis.sourceEntity!=null||basis.delegationDepth!=null;
  if (basis.source===6 ? basis.resolvedSource==null||basis.sourceEntity==null||basis.delegationDepth==null : any)
    ctx.addIssue({code:z.ZodIssueCode.custom,message:"CS2 delegated eye basis requires exactly its three source metadata fields"});
}).nullable();
const punch = z.object({
  predictable: z.object({ baseTime: gameTime, baseAngle: angle, baseAngularVelocity: angle }).strict().nullable(),
  unpredictable: z.object({ baseTick: z.number().int().min(-2147483648).max(2147483647).nullable(), baseAngle: angle }).strict().nullable(),
  times: z.object({
    worldGroup: z.number().int().min(0).max(4294967295).nullable(), visualPolicy: z.number().int().min(0).max(2),
    nativeContext: context,
    secondsPerTick: value, simulation: gameTime, visualPredictable: gameTime, visualUnpredictable: gameTime,
  }).strict().nullable(),
}).strict().nullable();

export const cs2RecoilTelemetrySchema = z.object({
  readOnly: z.literal(true), buildSupported: z.boolean(),
  mouse: z.object({
    source: z.number().int().min(0).max(2), sensitivity: value, verticalScale: value, yaw: value, pitch: value,
    zoomRatio: value, invertedY: z.boolean().nullable(), effectivePitch: value, pawnFovAdjust: value,
    pawnMouseSensitivity: value, yawDegreesPerDelta: value, pitchDegreesPerDelta: value,
  }).strict().nullable(),
  crosshairEnabled: z.boolean().nullable(), localPunch: punch, cameraPunch: punch,
  networkEyeAngles: angle, inputEyeAngles: angle, localEyeBasis: eyeBasis, cameraEyeBasis: eyeBasis,
  localBasisContextMatches: z.boolean().nullable(),
  directionContextIndependent: z.boolean().optional(),
  cameraPawn: z.string().regex(/^\d+$/).nullable(), cameraPath: z.number().int().min(0).max(2),
  cameraViewState: z.object({ baseAngle: angle, baseTime: gameTime, evaluationSeconds: value, decayExponent: value }).strict().nullable(),
  evaluated: z.object({
    shotPredictable: angle, shotUnpredictable: angle, shotOffset: angle, rawEyePlusRecoil: angle, contextDirection: angle,
    crosshairFollow: angle, cameraPredictable: angle, cameraUnpredictable: angle, cameraAim: angle,
    cameraView: angle, cameraFeedback: angle,
  }).strict(),
}).strict().superRefine((state, context) => {
  const basis = state.localEyeBasis, nativeContext = state.localPunch?.times?.nativeContext;
  const independent = state.directionContextIndependent === true && basis?.source === 2 &&
      basis.interpolationParameter == null && state.cameraPath === 1 && state.cameraPawn !== null;
  if (state.directionContextIndependent === true && !independent)
    context.addIssue({code:z.ZodIssueCode.custom,path:["directionContextIndependent"],message:"Only the verified ordinary local input branch is selector-independent"});
  if (state.evaluated.contextDirection === null) return;
  const matching = basis?.nativeContext != null && nativeContext != null &&
      basis.nativeContext === nativeContext && state.localBasisContextMatches === true;
  const timePresent=(t:z.infer<typeof gameTime>|undefined)=>t?.tick!=null&&t.fraction!=null;
  const p=state.localPunch;
  const completeShot=p?.predictable?.baseAngle!=null&&p.predictable.baseAngularVelocity!=null&&
      timePresent(p.predictable.baseTime)&&p.unpredictable?.baseAngle!=null&&p.unpredictable.baseTick!=null&&
      p.times?.worldGroup!=null&&p.times.secondsPerTick===1/64&&timePresent(p.times.simulation);
  if (!state.buildSupported || !basis?.value || basis.source === 0 || (!matching && !independent) ||
      !completeShot || state.evaluated.shotOffset === null) {
    context.addIssue({code: z.ZodIssueCode.custom, path: ["evaluated", "contextDirection"], message: "CS2 direction requires matching contexts or a verified independent local input branch"});
  }
});
export type Cs2RecoilTelemetry = z.infer<typeof cs2RecoilTelemetrySchema>;
