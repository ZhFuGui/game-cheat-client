import type { RuntimeState } from "@mousebox/shared";

const applicationKeys = new Set(["boxReady", "boxFeedbackAgeMs", "boxFeedbackSequence", "boxReceiverReady", "boxButtonsKnown", "button", "boxStatus", "deviceCapabilitiesKnown", "deviceBaudRate", "deviceMaximumStepCounts", "deviceMinimumIntervalMs", "deviceMaximumCountsPerSecond", "renderer", "adapter", "adapterId", "adapterNotice", "graphicsError", "adapterReady", "sessionEligible"]);
export function retainApplicationState(previous: RuntimeState): RuntimeState {
  const state: RuntimeState = Object.fromEntries(Object.entries(previous).filter(([key]) => applicationKeys.has(key)));
  if (previous.connections) state.connections = { ...previous.connections, reader: { phase: "closed", message: "游戏已切换，等待当前数据" } };
  return state;
}
