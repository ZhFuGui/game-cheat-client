import {
  applicationSettingsSchema,
  espSettingsSchema,
  type EspKey,
  type SettingKey,
} from "@mousebox/shared";
import { toast } from "sonner";
import { useTranslation } from "react-i18next";
import type { SettingValidator } from "~/games/contracts";
import { getGameSettingValidator } from "~/games/registry";
import { useRuntimeStore, type RuntimeStore } from "~/runtime/state";
import { useCommand } from "./useCommand";

export type SettingTarget =
  | { setting: SettingKey; esp?: false }
  | { setting: EspKey; esp: true };

const applicationValidators: Partial<Record<SettingKey, SettingValidator>> = applicationSettingsSchema.shape;
const espValidators: Record<EspKey, SettingValidator> = espSettingsSchema.shape;

function selectSettingValue(store: RuntimeStore, target: SettingTarget): unknown {
  if (target.esp) return store.espValues?.[target.setting];
  const values: Partial<Record<SettingKey, unknown>> | null = applicationValidators[target.setting]
    ? store.applicationValues
    : store.values;
  return values?.[target.setting];
}

// Controls bind to settings ownership; each game supplies its own field rules.
export function useSettingBinding(target: SettingTarget) {
  const { run } = useCommand();
  const { t } = useTranslation();
  const value = useRuntimeStore(store => selectSettingValue(store, target));
  const gameId = useRuntimeStore(store => store.workspace?.active?.gameId);
  const validator = target.esp
    ? espValidators[target.setting]
    : applicationValidators[target.setting] ?? getGameSettingValidator(gameId, target.setting);
  const save = async (requested: unknown) => {
    if (!validator?.safeParse(requested).success) {
      toast.error(t("请输入有效值"));
      return undefined;
    }
    return target.esp
      ? run("esp.set", { key: target.setting, value: requested })
      : run("settings.set", { key: target.setting, value: requested });
  };
  return { value, validator, save };
}
