import { z } from "zod";
import { runtimeStateSchema, connectionIndicatorSchema, type RuntimeState } from "./runtime";

export const deviceStateSchema = runtimeStateSchema.pick({
  boxReady: true, boxFeedbackAgeMs: true, boxFeedbackSequence: true,
  boxReceiverReady: true, boxButtonsKnown: true, button: true, boxStatus: true,
  deviceCapabilitiesKnown: true, deviceBaudRate: true, deviceMaximumStepCounts: true,
  deviceMinimumIntervalMs: true, deviceMaximumCountsPerSecond: true, testArmed: true,
  renderer: true, adapter: true, adapterId: true, adapterNotice: true,
  graphicsError: true, adapterReady: true,
}).extend({
  connections: z.object({ serial: connectionIndicatorSchema, box: connectionIndicatorSchema }).strict().optional(),
}).strict();
export type DeviceState = z.infer<typeof deviceStateSchema>;
export const deviceStateKeys = new Set(Object.keys(deviceStateSchema.shape));

/** Compatibility projection. Explicitly exclude Reader and all game facts. */
export function pickDeviceState(state: RuntimeState): DeviceState {
  const result: Record<string, unknown> = Object.fromEntries(
    Object.entries(state).filter(([key]) => key !== "connections" && deviceStateKeys.has(key)),
  );
  if (state.connections) result.connections = { serial: state.connections.serial, box: state.connections.box };
  return deviceStateSchema.parse(result);
}
export function combineRuntimeState(state: RuntimeState, devices: DeviceState): RuntimeState {
  const result: RuntimeState = { ...state, ...devices, connections: state.connections };
  if (devices.connections) result.connections = {
    reader: state.connections?.reader ?? { phase: "closed", message: "等待游戏数据源" },
    ...devices.connections,
  };
  return result;
}

