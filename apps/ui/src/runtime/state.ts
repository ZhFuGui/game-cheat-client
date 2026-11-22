import { create } from "zustand";
import { retainApplicationState } from "./workspaceState";
import {
  LOG_LIMIT,
  type Algorithm,
  type Bootstrap,
  type LearningState,
  type MouseReportsState,
  type LogBatch,
  type LogEntry,
  type NativeSnapshot,
  type RuntimeState,
  type UiPreferences,
  type WorkspaceCatalog,
  type ApplicationSettings,
  type DeviceState,
  pickDeviceState,
  combineRuntimeState,
  sameWorkspaceContext,
} from "@mousebox/shared";

export interface RuntimeStore {
  ready: boolean;
  connected: boolean;
  error: string | null;
  version: string;
  session: string;
  values: NativeSnapshot["values"] | null;
  applicationValues: ApplicationSettings | null;
  applicationProvided: boolean;
  deviceState: DeviceState;
  devicesProvided: boolean;
  speedPresets: NativeSnapshot["speedPresets"];
  espValues: NativeSnapshot["espValues"] | null;
  state: RuntimeState;
  learning: LearningState | null;
  mouseReports: MouseReportsState | null;
  algorithm: Algorithm;
  algorithmSections: NativeSnapshot["algorithmSections"];
  ports: NativeSnapshot["ports"];
  graphicsAdapters: NativeSnapshot["graphicsAdapters"];
  preferences: UiPreferences;
  logs: LogEntry[];
  logCursor: string;
  migration: Bootstrap["migration"];
  workspace: WorkspaceCatalog | undefined;
  workspaceRefreshing: boolean;
  applyWorkspace: (catalog: WorkspaceCatalog) => void;
  initialize: (value: Bootstrap) => void;
  setError: (error: string | null) => void;
  appendLogs: (batch: LogBatch) => void;
}
export const useRuntimeStore = create<RuntimeStore>((set, get) => ({
  ready: false,
  connected: false,
  error: null,
  version: "",
  session: "",
  values: null,
  applicationValues: null,
  applicationProvided: false,
  deviceState: {},
  devicesProvided: false,
  speedPresets: [],
  espValues: null,
  state: {},
  learning: null,
  mouseReports: null,
  algorithm: {},
  algorithmSections: [],
  ports: [],
  graphicsAdapters: [],
  preferences: { language: "zh", colorMode: "light" },
  logs: [],
  logCursor: "",
  migration: undefined,
  workspace: undefined,
  workspaceRefreshing: false,
  applyWorkspace: (workspace) => {
    const current = get();
    const changed = !!current.workspace && !(current.workspace.active === null && workspace.active === null) && !sameWorkspaceContext(current.workspace.active, workspace.active);
    if (!changed) { set({ workspace }); return; }
    // Device and GPU facts survive a game change; Reader, targets, tasks and scenes do not.
    const state = combineRuntimeState(retainApplicationState(current.state), current.deviceState);
    set({ workspace, workspaceRefreshing: !!workspace.active, values: null, espValues: null, state, learning: null, mouseReports: null, algorithm: {}, algorithmSections: [] });
  },
  initialize: (v) =>
    set({
      values: v.values,
      applicationValues: v.application ?? { dpi: v.values.dpi, renderAdapter: v.values.renderAdapter },
      applicationProvided: !!v.application,
      speedPresets: v.speedPresets,
      espValues: v.espValues,
      state: combineRuntimeState(v.state, v.devices ?? pickDeviceState(v.state)),
      deviceState: v.devices ?? pickDeviceState(v.state),
      devicesProvided: !!v.devices,
      learning: v.learning,
      mouseReports: v.mouseReports,
      algorithm: v.algorithm,
      algorithmSections: v.algorithmSections,
      ports: v.ports,
      graphicsAdapters: v.graphicsAdapters,
      preferences: v.preferences,
      version: v.version,
      session: v.session,
      migration: v.migration,
      workspace: v.workspace,
      workspaceRefreshing: false,
      ready: true,
      connected: true,
      error: null,
    }),
  setError: (error) => set({ error }),
  appendLogs: (batch) => {
    const old = get().logs;
    const seen = new Set(old.map((v) => v.id));
    const entries = batch.entries.filter((v) => {
      if (seen.has(v.id)) return false;
      seen.add(v.id);
      return true;
    });
    set({
      logs: [...old, ...entries].slice(-LOG_LIMIT),
      logCursor: batch.cursor,
    });
  },
}));
