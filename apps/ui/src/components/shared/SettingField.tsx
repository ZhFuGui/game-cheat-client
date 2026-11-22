import { useEffect, useId, useRef, useState, type ReactNode } from "react";
import {HelpTooltip} from "./HelpTooltip";
import { useTranslation } from "react-i18next";
import { Input } from "~/components/ui/input";
import { Select, type Option } from "~/components/ui/select";
import { Switch } from "~/components/ui/switch";
import { useSettingBinding, type SettingTarget } from "~/hooks/useSettingBinding";

export function SettingField(props: SettingTarget & {
  label: string;
  help?: ReactNode;
  suffix?: string;
  numeric?: boolean;
  disabled?: boolean;
  onCommitted?: (value: string | number) => Promise<void>;
}) {
  const { label, help, suffix, numeric = true, disabled = false, onCommitted } = props;
  const id = useId(),
    { t } = useTranslation();
  const { value, validator, save } = useSettingBinding(props);
  const [draft, setDraft] = useState(value == null ? "" : String(value)),
    [error, setError] = useState("");
  const focused = useRef(false),
    cancelled = useRef(false);
  useEffect(() => {
    if (!focused.current) setDraft(value == null ? "" : String(value));
  }, [value]);
  const commit = async () => {
    focused.current = false;
    if (cancelled.current) {
      cancelled.current = false;
      return;
    }
    const requested = numeric
      ? draft.trim() === ""
        ? Number.NaN
        : Number(draft)
      : draft.trim();
    if (!validator?.safeParse(requested).success) {
      setError(t("请输入有效值"));
      return;
    }
    setError("");
    if (requested === value) return;
    const result = await save(requested);
    if (result && onCommitted) await onCommitted(requested);
  };
  return (
    <div className="field">
      <div className="field-label"><label htmlFor={id}>{t(label)}</label>{help&&<HelpTooltip label={label}>{help}</HelpTooltip>}</div>
      <div className="input-wrap">
        <Input
          id={id}
          type="text"
          inputMode={numeric ? "decimal" : undefined}
          value={draft}
          disabled={disabled || value == null}
          onFocus={() => {
            focused.current = true;
          }}
          onChange={(e) => {
            setDraft(e.target.value);
            setError("");
          }}
          onBlur={() => void commit()}
          onKeyDown={(e) => {
            if (e.key === "Enter") e.currentTarget.blur();
            if (e.key === "Escape") {
              cancelled.current = true;
              setDraft(value == null ? "" : String(value));
              setError("");
              e.currentTarget.blur();
            }
          }}
          aria-invalid={!!error}
          aria-describedby={error ? `${id}-error` : undefined}
        />
        {suffix && <span className="input-suffix">{suffix}</span>}
      </div>
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
}) {
  const { label, help, options, disabled = false } = props;
  const id = useId(),
    { t } = useTranslation();
  const { value, save } = useSettingBinding(props);
  return (
    <div className="field">
      <div className="field-label"><label htmlFor={id}>{t(label)}</label>{help&&<HelpTooltip label={label}>{help}</HelpTooltip>}</div>
      <Select
        id={id}
        label={t(label)}
        value={(value as string | number | undefined) ?? ""}
        options={options.map((o) => ({ ...o, label: t(o.label) }))}
        disabled={disabled || value === undefined}
        onChange={(requested) => void save(requested)}
      />
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
