import { combineRuntimeState, type DeviceState, type NativeSnapshot, type UiPreferences } from "@mousebox/shared";
import { localizeDisplay } from "~/i18n/displayText";
import { retainApplicationState } from "./workspaceState";

type DisplayFields = Partial<
  Pick<
    NativeSnapshot,
    | "state"
    | "learning"
    | "algorithm"
    | "algorithmSections"
    | "ports"
    | "graphicsAdapters"
  >
>;

// Retain native text as the source of truth. Translating already translated
// values would lose information when preferences switch back to Chinese.
export class RuntimeDisplayProjection {
  private readonly source: DisplayFields = {};
  private devices: DeviceState = {};

  remember(value: DisplayFields): void {
    Object.assign(this.source, value);
    if (value.state) this.source.state = combineRuntimeState(value.state, this.devices);
  }

  resetWorkspace(): void {
    if (this.source.state) this.source.state = retainApplicationState(this.source.state);
    delete this.source.learning;
    delete this.source.algorithm;
    delete this.source.algorithmSections;
  }
  rememberDevices(devices: DeviceState): void {
    this.devices = devices;
    this.source.state = combineRuntimeState(this.source.state ?? {}, devices);
  }
  devicesInLanguage(language: UiPreferences["language"]): DeviceState {
    return this.translate(this.devices, language);
  }

  translate<T>(value: T, language: UiPreferences["language"]): T {
    return localizeDisplay(value, language);
  }

  inLanguage(language: UiPreferences["language"]): DisplayFields {
    return this.translate(this.source, language);
  }
}
