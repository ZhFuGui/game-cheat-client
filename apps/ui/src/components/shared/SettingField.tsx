import { useEffect, useId, useRef, useState, type ReactNode } from "react";
import {HelpTooltip} from "./HelpTooltip";
import { useTranslation } from "react-i18next";
import { ChoiceButtons } from "~/components/ui/choice-buttons";
import { Input } from "~/components/ui/input";
import { Select, type Option } from "~/components/ui/select";
import { Slider, type SliderRange } from "~/components/ui/slider";
import { Switch } from "~/components/ui/switch";
import { useSettingBinding, type SettingTarget } from "~/hooks/useSettingBinding";

export function SettingField(props: SettingTarget & {
  label: string;
  help?: ReactNode;
  suffix?: string;
  slider?: SliderRange;
  numeric?: boolean;
  disabled?: boolean;
  onCommitted?: (value: string | number) => Promise<void>;
}) {
  const { label, help, suffix, numeric = true, disabled = false, onCommitted, slider } = props;
  const id = useId(),
    { t } = useTranslation();
  const { value, validator, save, pending } = useSettingBinding(props);
  const [draft, setDraft] = useState<string | null>(null),
    [error, setError] = useState("");
  const [preview, setPreview] = useState<number | null>(null);
  const focused = useRef(false),
    cancelled = useRef(false);
  const sliderEdited = useRef(false);
  const range = numeric ? slider : undefined;
  useEffect(() => {
    if (!focused.current) setDraft(null);
  }, [value]);
  const text = draft ?? String(value ?? "");
  const numberDraft = text.trim() === "" ? Number.NaN : Number(text);
  const commitValue = async (requested: string | number, submittedDraft: string) => {
    if (range && pending) return;
    if (!validator?.safeParse(requested).success) {
      setError(t("请输入有效值"));
      return;
    }
    setError("");
    const clearDraft = () => setDraft(current => current === submittedDraft ? null : current);
    if (requested === value) {
      clearDraft();
      return;
    }
    const result = await save(requested);
    if (result || range) clearDraft();
    if (result) await onCommitted?.(requested);
  };
  const commitText = () => {
    focused.current = false;
    // Moving focus to the slider during a drag must not submit its preview.
    if (cancelled.current || sliderEdited.current) {
      cancelled.current = false;
      return;
    }
    if (draft !== null) void commitValue(numeric ? numberDraft : draft.trim(), draft);
  };
  const input = (
    <div className="input-wrap">
      <Input
        id={id}
        type="text"
        inputMode={numeric ? "decimal" : undefined}
        value={preview === null ? text : String(preview)}
        disabled={disabled || value == null}
        readOnly={!!range && pending}
        onFocus={() => {
          focused.current = true;
        }}
        onChange={(e) => {
          setDraft(e.target.value);
          setError("");
        }}
        onBlur={commitText}
        onKeyDown={(e) => {
          if (e.key === "Enter") e.currentTarget.blur();
          if (e.key === "Escape") {
            cancelled.current = true;
            setDraft(null);
            setError("");
            e.currentTarget.blur();
          }
        }}
        aria-invalid={!!error}
        aria-describedby={error ? `${id}-error` : undefined}
      />
      {suffix && <span className="input-suffix">{suffix}</span>}
    </div>
  );
  const fieldLabel = t(label);
  const sliderCandidate = preview ?? (range && validator?.safeParse(numberDraft).success ? numberDraft : value);
  return (
    <div className="field">
      <div className="field-label"><label htmlFor={id}>{fieldLabel}</label>{help && <HelpTooltip label={label}>{help}</HelpTooltip>}</div>
      {range ? <div className="setting-slider-pair" aria-busy={pending || undefined}>
        <Slider
          {...range}
          value={typeof sliderCandidate === "number" ? Math.min(range.max, Math.max(range.min, sliderCandidate)) : range.min}
          labels={[t("调节 {{field}}", { field: fieldLabel })]}
          unit={suffix}
          disabled={disabled || typeof value !== "number" || !validator}
          onValueChange={(next, details) => {
            if (pending) {
              details.cancel();
              return;
            }
            sliderEdited.current = true;
            setPreview(next);
            setError("");
          }}
          onValueCommitted={(next) => {
            if (!sliderEdited.current) return;
            sliderEdited.current = false;
            setPreview(null);
            const nextText = String(next);
            setDraft(nextText);
            void commitValue(next, nextText);
          }}
          onCancel={() => {
            if (!sliderEdited.current) return;
            sliderEdited.current = false;
            setPreview(null);
            setDraft(null);
            setError("");
          }}
        />
        {input}
      </div> : input}
      {error && (
        <small id={`${id}-error`} className="field-error">
          {error}
        </small>
      )}
    </div>
  );
}
export function SettingChoice(props: SettingTarget & {
  label: string;
  help?: ReactNode;
  options: Option[];
  disabled?: boolean;
  presentation?: "buttons";
}) {
  const { label, help, options, disabled = false, presentation } = props;
  const id = useId(),
    { t } = useTranslation();
  const { value, save } = useSettingBinding(props);
  const fieldLabel = t(label);
  const translatedOptions = options.map(option => ({ ...option, label: t(option.label) }));
  const selection = (value as string | number | undefined) ?? "";
  const unavailable = disabled || value === undefined;
  const select = (requested: string | number) => void save(requested);
  return (
    <div className="field">
      <div className="field-label">
        {presentation === "buttons" ? <span>{fieldLabel}</span> : <label htmlFor={id}>{fieldLabel}</label>}
        {help && <HelpTooltip label={label}>{help}</HelpTooltip>}
      </div>
      {presentation === "buttons" ? <ChoiceButtons
        label={fieldLabel}
        value={selection}
        options={translatedOptions}
        disabled={unavailable}
        onChange={select}
      /> : <Select
        id={id}
        label={fieldLabel}
        value={selection}
        options={translatedOptions}
        disabled={unavailable}
        onChange={select}
      />}
    </div>
  );
}
export function SettingToggle(props: SettingTarget & {
  label: string;
  help?: ReactNode;
  disabled?: boolean;
}) {
  const { label, help, disabled = false } = props;
  const id = useId(),
    { t } = useTranslation();
  const { value, save } = useSettingBinding(props);
  return (
    <div className="toggle-row">
      <div className="field-label"><label htmlFor={id}>{t(label)}</label>{help&&<HelpTooltip label={label}>{help}</HelpTooltip>}</div>
      <Switch
        id={id}
        checked={(value as boolean | undefined) ?? false}
        disabled={disabled || value === undefined}
        onCheckedChange={(requested) => void save(requested)}
      />
    </div>
  );
}
