import type { GameId, SettingKey } from "@mousebox/shared";
import { pubgUiModule } from "./pubg";
import { cs2UiModule } from "./cs2";
import type { GameUiModule } from "./contracts";

// A directory entry never substitutes for a real game implementation.
const modules: Readonly<Partial<Record<GameId, GameUiModule>>> = {
  pubg: pubgUiModule,
  cs2: cs2UiModule,
};

export function getGameUiModule(id: GameId | undefined): GameUiModule | undefined {
  return id ? modules[id] : undefined;
}

export function getGameSettingValidator(id: GameId | undefined, key: SettingKey) {
  return getGameUiModule(id)?.settingValidators[key];
}

