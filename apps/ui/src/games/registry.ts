import type { GameCapability, GameId, SettingKey } from "@mousebox/shared";
import { pubgUiModule } from "./pubg";
import { cs2UiModule } from "./cs2";
import { valorantUiModule } from "./valorant";
import type { GameUiModule } from "./contracts";

// A directory entry never substitutes for a real game implementation.
const modules: Readonly<Partial<Record<GameId, GameUiModule>>> = {
  pubg: pubgUiModule,
  cs2: cs2UiModule,
  valorant: valorantUiModule,
};

export function getGameUiModule(id: GameId | undefined): GameUiModule | undefined {
  return id ? modules[id] : undefined;
}

export function getGameSettingValidator(id: GameId | undefined, key: SettingKey) {
  return getGameUiModule(id)?.settingValidators[key];
}

export function resolveGameSettings(game: GameId | undefined, capabilities: readonly GameCapability[] | undefined, section: string | null) {
  const panes = getGameUiModule(game)?.settings.filter(pane => !pane.capability || capabilities?.includes(pane.capability)) ?? [];
  return { panes, selected: panes.find(pane => pane.id === section) ?? panes[0] };
}

