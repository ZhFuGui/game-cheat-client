import { useTranslation } from "react-i18next";
import { ChoiceButtons } from "~/components/ui/choice-buttons";
import { useRuntimeStore } from "~/runtime/state";
import { useCommand } from "~/hooks/useCommand";
export function General() {
  const { t } = useTranslation(),
    colorMode = useRuntimeStore((s) => s.preferences.colorMode),
    language = useRuntimeStore((s) => s.preferences.language),
    { run, pending } = useCommand();
  return (
    <div className="preferences">
      <div className="preference-row">
        <span>{t("外观")}</span>
        <ChoiceButtons
          label={t("外观")}
          value={colorMode}
          disabled={pending}
          onChange={(colorMode) => void run("preferences.set", { colorMode })}
          options={[
            { label: t("浅色"), value: "light", icon: "lightTheme" },
            { label: t("深色"), value: "dark", icon: "darkTheme" },
          ]}
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
