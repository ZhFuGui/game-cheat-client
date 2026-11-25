import {useId, useState, type ReactNode} from "react";
import {useTranslation} from "react-i18next";
import {type ExecutableMouseMovementAlgorithm, type MouseMovementDefinition} from "@mousebox/shared";
import {FeatureSection} from "~/components/shared/FeatureSection";
import {HelpTooltip} from "~/components/shared/HelpTooltip";
import {Select} from "~/components/ui/select";
import {useSettingBinding} from "~/hooks/useSettingBinding";
import {useRuntimeStore} from "~/runtime/state";
import {MouseBezierSettings, MouseTrajectoryChoice} from "./MouseTrajectorySettings";

export function MouseMovementAlgorithms({choices, defaultSelection, children}: {
  choices: readonly MouseMovementDefinition[];
  defaultSelection: ExecutableMouseMovementAlgorithm;
  children?: ReactNode;
}) {
  const {t} = useTranslation();
  const speedId = useId();
  const {value, save} = useSettingBinding({setting: "mouseMovementAlgorithm"});
  const ready = useRuntimeStore(s => !!s.values);
  const calibrating = useRuntimeStore(s => !!s.state.calibrating || !!s.state.cs2Controller?.calibrating);
  const [saving, setSaving] = useState(false);
  const selected = choices.find(choice => choice.id === value) ?? choices.find(choice => choice.id === defaultSelection);
  const choose = async (requested: string | number) => {
    if (requested === value) return;
    setSaving(true);
    try { await save(String(requested)); } finally { setSaving(false); }
  };
  return <FeatureSection title="鼠标移动算法" className="mouse-movement-algorithms">
    <div className="aim-fields">
      <MouseTrajectoryChoice disabled={!ready || saving || calibrating}/>
      <div className="field">
        <div className="field-label"><label htmlFor={speedId}>{t("速度变化控制")}</label>
          {selected && <HelpTooltip label="速度变化控制">{t(selected.description)}</HelpTooltip>}
        </div>
        <Select id={speedId} value={selected?.id ?? defaultSelection}
          disabled={!ready || saving || calibrating || choices.length < 2}
          options={choices.map(choice => ({value: choice.id, label: t(choice.title)}))}
          onChange={requested => void choose(requested)}/>
      </div>
      <MouseBezierSettings disabled={!ready || saving || calibrating}/>
      {children}
    </div>
  </FeatureSection>;
}
