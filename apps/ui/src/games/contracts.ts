import type { ComponentType } from "react";
import type { GameCapability, GameId, SettingKey } from "@mousebox/shared";

export interface SettingValidator {
  safeParse(value: unknown): { success: boolean };
}

export interface GameEspSettingsProps {
  tab: string;
  setTab: (value: string) => void;
}

export interface GameSettingsPane {
  id: string;
  title: string;
  description: string;
  capability?: GameCapability;
  component: ComponentType;
}
export type GameExtensionSlot = "esp-settings" | "game-settings";
export interface GameExtensionUi {
  id: string;
  slots: Partial<Record<GameExtensionSlot, ComponentType>>;
}
/** A game registers its real shared Aim modules; each module owns its existing settings. */
export interface AimGeneralModule {
  id:string;
  component:ComponentType;
}
export interface GameUiModule {
  id: GameId;
  settingValidators: Readonly<Partial<Record<SettingKey, SettingValidator>>>;
  AimSettings?: ComponentType;
  aimGeneral?: readonly AimGeneralModule[];
  Triggerbot?: ComponentType;
  Rage?: ComponentType;
  navigationHelp?:Readonly<Record<string,{title:string;paragraphs:readonly string[]}>>;
  SourcePanel: ComponentType;
  DataPanel: ComponentType;
  EspSettings?: ComponentType<GameEspSettingsProps>;
  Radar?: ComponentType;
  VisualsSettings?: ComponentType;
  externalEsp?: boolean;
  settings: readonly GameSettingsPane[];
  extensions?: readonly GameExtensionUi[];
}

