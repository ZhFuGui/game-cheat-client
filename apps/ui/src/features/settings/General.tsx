import { useTranslation } from "react-i18next";
import { AppIcon } from "~/components/icons";
import { ChoiceButtons } from "~/components/ui/choice-buttons";
import { Switch } from "~/components/ui/switch";
import { useRuntimeStore } from "~/runtime/state";
import { useCommand } from "~/hooks/useCommand";
export function General() {
  const { t } = useTranslation(),
    colorMode = useRuntimeStore((s) => s.preferences.colorMode),
    language = useRuntimeStore((s) => s.preferences.language),
    { run, pending } = useCommand();
  const dark = colorMode === "dark";
  const themeLabel = t("深色模式");
  return (
    <div className="preferences">
      <div className="preference-row">
        <span>{t("外观")}</span>
        <Switch
          className="theme-switch"
          aria-label={themeLabel}
          title={themeLabel}
          checked={dark}
          disabled={pending}
          onCheckedChange={(checked) => void run("preferences.set", { colorMode: checked ? "dark" : "light" })}
          thumbContent={<AppIcon name={dark ? "darkTheme" : "lightTheme"} size={13} />}
        />
      </div>
      <div className="preference-row">
        <span>{t("语言")}</span>
        <ChoiceButtons
          label={t("语言")}
          value={language}
          disabled={pending}
          onChange={(language) => void run("preferences.set", { language })}
          options={[
            { value: "zh", label: "简体中文" },
            { value: "en", label: "English" },
          ]}
        />
      </div>
    </div>
  );
}
