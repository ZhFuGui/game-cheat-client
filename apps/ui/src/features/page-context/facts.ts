import type { LogEntry, RuntimeState } from "@mousebox/shared";
import {cs2TriggerMessages,cs2RageMessages} from "@mousebox/shared";
import type { RuntimeStore } from "~/runtime/state";
import type { ContextPage } from "./model";

type Phase = NonNullable<RuntimeState["connections"]>["reader"]["phase"];
export interface ContextFacts {
  ready: boolean;
  connected: boolean;
  runtimeError: string;
  migrationRequired: boolean;
  sessionEligible?: boolean;
  readerPhase?: Phase;
  readerMessage?: string;
  serialPhase?: Phase;
  serialMessage?: string;
  boxPhase?: Phase;
  boxMessage?: string;
  readerRunning?: boolean;
  readerConnected?: boolean;
  boxReady?: boolean;
  host?: string;
  port?: number;
  hz?: number;
  characters?: number | string | boolean | null;
  fov?: number | string | boolean | null;
  control?: string;
  canAim?: boolean;
  aimArmed?: boolean;
  aimEnabled?: boolean;
  target?: number | string | boolean | null;
  distance?: number | string | boolean | null;
  ballistics?: string;
  ballisticsError?: string;
  ballisticProfile?: string;
  espVisible?: string;
  adapter?: string;
  adapterReady?: boolean;
  adapterNotice?: string;
  graphicsError?: string;
  calibrating?: boolean;
  calibration?: string;
  testArmed?: boolean;
  dpi?: number;
  mapping?: number;
  speedLimit?: number;
  recording?: boolean;
  training?: boolean;
  ioBusy?: boolean;
  replaying?: boolean;
  modelActive?: boolean;
  canApply?: boolean;
  sessions?: number;
  trials?: number;
  samples?: number;
  learningFeedback?: string;
  reports?: number;
  elapsedMs?: number;
  analyzing?: boolean;
  reportsBusy?: boolean;
  hasResult?: boolean;
  reportReason?: string;
  reportError?: string;
  logCount?: number;
  logWarnings?: number;
  logErrors?: number;
  language?: string;
  colorMode?: string;
  workspaceEpoch?: string;
  workspaceSwitching?: boolean;
  recoilOnly?:boolean;
  triggerEnabled?:boolean;
  triggerBlock?:string;
  triggerReason?:string;
  triggerClicks?:number;
  triggerTarget?:number|null;
  triggerHitbox?:number|null;
  rageEnabled?:boolean;
  rageActive?:boolean;
  rageBlock?:string;
  rageReason?:string;
  rageTarget?:number|null;
  rageDamage?:number|null;
  rageClicks?:number;
  extensionId?:string;
  extensionPhase?:string;
  extensionCode?:string;
  extensionMessage?:string;
}
const readerPages = new Set<ContextPage>(["reader", "dashboard", "esp", "aimbot", "rage", "input", "algorithm", "calibration"]);
const boxPages = new Set<ContextPage>(["aimbot", "rage", "firmware", "calibration", "input-device"]);
const drawingPages = new Set<ContextPage>(["esp", "rendering"]);
const logSummaries = new WeakMap<LogEntry[], { logCount: number; logWarnings: number; logErrors: number }>();
function logSummary(logs: LogEntry[]) {
  let result = logSummaries.get(logs);
  if (!result) {
    result = { logCount: logs.length, logWarnings: 0, logErrors: 0 };
    for (const entry of logs) {
      if (entry.level === "warning") result.logWarnings++;
      if (entry.level === "error") result.logErrors++;
    }
    logSummaries.set(logs, result);
  }
  return result;
}
/** Primitive, route-specific subscriptions exclude raw input, traces and unrelated telemetry. */
export function selectContextFacts(s: RuntimeStore, page: ContextPage): ContextFacts {
  const f: ContextFacts = {
    ready: s.ready, connected: s.connected, runtimeError: !s.connected ? s.error ?? "" : "",
    migrationRequired: s.migration?.required ?? false,
    workspaceEpoch: s.workspace?.active?.epoch,
    workspaceSwitching: s.workspaceRefreshing,
  };
  if (!s.ready || !s.connected || f.migrationRequired) return f;
  const state = s.state;
  f.sessionEligible = state.sessionEligible !== false;
  if (readerPages.has(page)) {
    f.readerPhase = state.connections?.reader.phase; f.readerMessage = state.connections?.reader.message;
    f.readerRunning = state.readerRunning; f.readerConnected = state.readerConnected;
  }
  if (boxPages.has(page)) {
    f.serialPhase = state.connections?.serial.phase; f.serialMessage = state.connections?.serial.message;
    f.boxPhase = state.connections?.box.phase; f.boxMessage = state.connections?.box.message;
    f.boxReady = state.boxReady;
  }
  if (drawingPages.has(page)) {
    f.adapter = state.adapter; f.adapterReady = state.adapterReady;
    f.adapterNotice = state.adapterNotice; f.graphicsError = state.graphicsError;
  }
  if (page === "reader") {
    f.host = s.values?.host; f.port = s.values?.port; f.hz = s.values?.hz;
  }
  if (page === "dashboard") f.characters = state.characters;
  if (page === "rage") {
    f.rageEnabled=state.cs2Rage?.enabled; f.rageActive=state.cs2Rage?.active;
    f.rageBlock=state.cs2Rage?.block; f.rageTarget=state.cs2Rage?.target;
    f.rageDamage=state.cs2Rage?.damage; f.rageClicks=state.cs2Rage?.clicks;
    f.rageReason=state.cs2Rage?.block?cs2RageMessages[state.cs2Rage.block]??state.cs2Rage.block:undefined;
  }
  if(page==="triggerbot") {f.triggerEnabled=state.cs2Trigger?.enabled;f.triggerBlock=state.cs2Trigger?.block;
    f.triggerClicks=state.cs2Trigger?.clicks;f.triggerTarget=state.cs2Trigger?.target;f.triggerHitbox=state.cs2Trigger?.hitbox;
    f.triggerReason=state.cs2Trigger?.block?cs2TriggerMessages[state.cs2Trigger.block]:undefined;}
  if (page === "esp") {
    f.espVisible = state.espVisible; f.fov = state.fov;
    const blocked = state.extensions?.filter(entry => entry.enabled && !["ready", "disabled", "paused"].includes(entry.phase));
    const extension = blocked?.find(entry => entry.phase === "error") ?? blocked?.find(entry => entry.phase === "degraded") ?? blocked?.[0];
    f.extensionId = extension?.id; f.extensionPhase = extension?.phase;
    f.extensionCode = extension?.code; f.extensionMessage = extension?.message;
  }
  if (page === "aimbot" || page === "algorithm") {
    f.aimEnabled = s.values?.aimEnabled !== false;
    f.control = state.control; f.canAim = state.canAim; f.aimArmed = state.aimArmed;
    f.recoilOnly = state.cs2Controller?.recoilOnly;
    f.target = state.target; f.distance = state.targetDistance;
    f.ballistics = state.ballistics; f.ballisticsError = state.ballisticsError;
    f.ballisticProfile = state.ballisticProfile;
  }
  if (page === "learning") {
    const l = s.learning;
    f.recording = l?.recording; f.training = l?.training; f.ioBusy = l?.ioBusy;
    f.replaying = l?.replaying; f.modelActive = l?.active; f.canApply = l?.canApply;
    f.sessions = l?.sessions; f.trials = l?.trials; f.samples = l?.samples;
    f.learningFeedback = l?.feedback;
  }
  if (page === "reports") {
    const r = s.mouseReports;
    f.recording = r?.recording; f.analyzing = r?.analyzing; f.reportsBusy = r?.busy;
    f.reports = r?.reports; f.elapsedMs = r?.elapsedMs; f.hasResult = r?.hasResult;
    f.reportReason = r?.reason; f.reportError = r?.error;
  }
  if (page === "logs") Object.assign(f, logSummary(s.logs));
  if (page === "general") { f.language = s.preferences.language; f.colorMode = s.preferences.colorMode; }
  if (page === "input" || page === "movement") {
    f.dpi = s.values?.dpi; f.mapping = s.values?.mapping; f.fov = state.fov;
    f.speedLimit = s.speedPresets.find(p => p.value === s.values?.speed)?.maximumCountsPerSecond;
  }
  if (page === "input-device") { f.dpi = s.values?.dpi; f.testArmed = state.testArmed; }
  if (page === "calibration") {
    f.calibrating = state.calibrating; f.calibration = state.calibration;
    f.testArmed = state.testArmed; f.mapping = s.values?.mapping;
  }
  return f;
}
