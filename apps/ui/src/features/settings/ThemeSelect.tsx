import { useTranslation } from "react-i18next";
import { Select } from "~/components/ui/select";
import { getThemePreset, themePresets } from "~/theme/presets";

export function ThemeSelect({ id, value, dark, disabled, onChange }: {
  id: string;
  value?: string;
  dark: boolean;
  disabled: boolean;
  onChange: (preset: string) => void;
}) {
  const { t } = useTranslation();
  return <div className="theme-picker">
    <Select
      id={id}
      label={t("主题")}
      value={getThemePreset(value).id}
      alignItemWithTrigger={false}
      options={themePresets.map(preset => ({
        value: preset.id,
        label: t(preset.label),
        leading: <span className={`theme-swatches${dark ? " dark" : ""}`} data-theme-preset={preset.id}>
          <i /><i /><i /><i />
        </span>,
      }))}
      onChange={preset => onChange(String(preset))}
      disabled={disabled}
    />
  </div>;
}
