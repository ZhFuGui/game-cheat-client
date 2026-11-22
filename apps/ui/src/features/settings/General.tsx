import {HelpTooltip} from "~/components/shared/HelpTooltip";
import { useTranslation } from "react-i18next";
import { Section } from "~/components/shared/Content";
import { ChoiceButtons } from "~/components/ui/choice-buttons";
import { useRuntimeStore } from "~/runtime/state";
import { useCommand } from "~/hooks/useCommand";
export function General() {
  const { t } = useTranslation(),
    colorMode = useRuntimeStore((s) => s.preferences.colorMode),
    language = useRuntimeStore((s) => s.preferences.language),
    { run, pending } = useCommand();
  return (
    <Section>
      <div className="preference-row">
        <div className="field-label">
          <span>{t("显示模式")}</span>
          <HelpTooltip label="显示模式"><p>{t("切换后立即生效，并在下次启动时保留。")}</p></HelpTooltip>
        </div>
        <ChoiceButtons
          label={t("显示模式")}
          value={colorMode}
          disabled={pending}
          onChange={(colorMode) => void run("preferences.set", { colorMode })}
          options={[
            { label: t("浅色"), value: "light" },
            { label: t("深色"), value: "dark" },
          ]}
        />
      </div>
      <div className="preference-row">
        <div className="field-label">
          <span>{t("界面语言")}</span>
          <HelpTooltip label="界面语言"><p>{t("主题和语言只改变界面，不改变控制参数。")}</p></HelpTooltip>
        </div>
        <ChoiceButtons
          label={t("界面语言")}
          value={language}
          disabled={pending}
          onChange={(language) => void run("preferences.set", { language })}
          options={[
            { value: "zh", label: "简体中文" },
            { value: "en", label: "English" },
          ]}
        />
      </div>
    </Section>
  );
}
