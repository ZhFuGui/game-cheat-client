import {useRef, useState} from "react";
import {useTranslation} from "react-i18next";
import {HelpTooltip} from "~/components/shared/HelpTooltip";
import {Slider} from "~/components/ui/slider";
import {useSettingBinding} from "~/hooks/useSettingBinding";
import {useRuntimeStore} from "~/runtime/state";

export function MovementSpeedFields({disabled=false}:{disabled?:boolean}) {
  const limitsKnown = useRuntimeStore(s => s.connected && s.state.deviceCapabilitiesKnown === true &&
    (s.state.deviceMaximumCountsPerSecond ?? 0) > 0);
  return limitsKnown ? <FollowSpeed disabled={disabled}/> : null;
}

function FollowSpeed({disabled}: {disabled: boolean}) {
  const {t} = useTranslation();
  const presets = useRuntimeStore(s => s.speedPresets).slice().sort((a, b) => a.percent - b.percent);
  const {value, validator, save, pending} = useSettingBinding({setting:"speed"});
  const [preview, setPreview] = useState<number | null>(null);
  const draft = useRef<number | null>(null);
  const selected = presets.find(preset => preset.value === (preview ?? value));
  if (!selected || !validator) return null;

  return <div className="field">
    <div className="field-label">
      <span>{t("跟随速度")}</span>
      <HelpTooltip label="跟随速度">{t("选择现有速度档位，百分比表示占当前设备速度上限的比例。")}</HelpTooltip>
    </div>
    <div className="setting-slider-pair" aria-busy={pending || undefined}>
      <Slider
        min={presets[0].percent}
        max={presets[presets.length - 1].percent}
        step={0.25}
        value={selected.percent}
        labels={[t("跟随速度")]}
        unit="%"
        disabled={disabled}
        onValueChange={(ratio, details) => {
          if (pending) {
            details.cancel();
            return;
          }
          const nearest = presets.reduce((closest, preset) =>
            Math.abs(preset.percent - ratio) < Math.abs(closest.percent - ratio) ? preset : closest);
          const next = details.reason === "keyboard" && nearest === selected
            ? presets[presets.indexOf(selected) + Math.sign(ratio - selected.percent)] ?? selected
            : nearest;
          draft.current = next.value;
          setPreview(next.value);
        }}
        onValueCommitted={async () => {
          const chosen = draft.current;
          draft.current = null;
          if (chosen !== null && chosen !== value) await save(chosen);
          setPreview(null);
        }}
        onCancel={() => {
          draft.current = null;
          setPreview(null);
        }}
      />
      <output>{selected.percent}%</output>
    </div>
  </div>;
}
