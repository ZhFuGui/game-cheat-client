import {useEffect, useId, useRef, useState, type ReactNode} from "react";
import {useTranslation} from "react-i18next";
import {Input} from "~/components/ui/input";
import {Slider, type SliderRange} from "~/components/ui/slider";
import {HelpTooltip} from "./HelpTooltip";
import {useSettingBinding, type SettingTarget} from "~/hooks/useSettingBinding";
import {type RandomRange} from "@mousebox/shared";
import {RandomDistributionSelect} from "./RandomDistributionSelect";

type RangeDraft = {min: string; max: string};
const formatDraft = (min?: number, max?: number): RangeDraft => ({min: String(min ?? ""), max: String(max ?? "")});
const parseEndpoint = (text: string) => text.trim() ? Number(text) : NaN;

export function SettingRangeField(props: SettingTarget & {label: string; help?: ReactNode; suffix?: string; slider: SliderRange}) {
  const {t} = useTranslation();
  const id = useId();
  const {value, validator, save, pending} = useSettingBinding(props);
  const range = value as RandomRange | undefined;
  const min = range?.min;
  const max = range?.max;
  const distribution = range?.distribution ?? "normal";
  const [draft, setDraft] = useState<RangeDraft | null>(null);
  const [invalid, setInvalid] = useState(false);
  const editing = useRef(false);
  const cancelled = useRef(false);
  const sliding = useRef(false);
  const label = t(props.label);
  const minLabel = `${label} ${t("下限")}`;
  const maxLabel = `${label} ${t("上限")}`;
  const text = draft ?? formatDraft(min, max);
  const candidate = {min: parseEndpoint(text.min), max: parseEndpoint(text.max), distribution};
  const validDraft = validator?.safeParse(candidate).success;
  const unavailable = min === undefined || max === undefined || !validator;

  useEffect(() => {
    if (!editing.current && !sliding.current) {
      setDraft(null);
      setInvalid(false);
    }
  }, [min, max]);

  const commit = async (requested: RandomRange, submitted: RangeDraft | null) => {
    if (pending) return;
    if (!validator?.safeParse(requested).success) {
      setInvalid(true);
      return;
    }
    setInvalid(false);
    if (requested.min !== min || requested.max !== max || requested.distribution !== distribution)
      await save(requested);
    setDraft(current => current === submitted ? null : current);
  };
  const endpoint = (bound: "min" | "max", label: string) => <Input
    value={text[bound]}
    type="text"
    inputMode="numeric"
    disabled={unavailable}
    readOnly={pending}
    aria-label={label}
    aria-invalid={invalid}
    aria-describedby={invalid ? id + "-error" : undefined}
    onChange={e => {
      const field = e.target.value;
      setDraft(current => ({...(current ?? formatDraft(min, max)), [bound]: field}));
      setInvalid(false);
    }}
    onKeyDown={e => {
      if (e.key === "Escape") {
        cancelled.current = true;
        setDraft(null);
        setInvalid(false);
      }
      if (e.key === "Enter" || e.key === "Escape")
        e.currentTarget.blur();
    }}
  />;

  return <div className="field">
    <div className="field-label">
      <span id={id}>{label}</span>
      <HelpTooltip label={props.label}>
        {props.help && <p>{props.help}</p>}
        <p>{t("正态分布集中在中点，标准差为区间宽度的六分之一；均匀分布让每个整数等概率；三角分布从中点向两端线性减少；偏向下限和偏向上限分别让较小值和较大值更常见。")}</p>
        <p>{t("所有模式均限制在上下限内，并取整为整数。上下限相同时使用固定值。")}</p>
      </HelpTooltip>
    </div>
    <div className="setting-random-range" aria-busy={pending || undefined}>
      <div className="setting-range-fields" role="group" aria-labelledby={id}
        onFocus={() => { editing.current = true; }}
        onBlur={e => {
          if (e.currentTarget.contains(e.relatedTarget)) return;
          editing.current = false;
          if (!cancelled.current && !sliding.current && draft) void commit(candidate, draft);
          cancelled.current = false;
        }}>
        {endpoint("min", minLabel)}
        <Slider<[number, number]>
          {...props.slider}
          value={validDraft ? [candidate.min, candidate.max] : [min ?? props.slider.min, max ?? props.slider.max]}
          labels={[t("调节 {{field}}", {field: minLabel}), t("调节 {{field}}", {field: maxLabel})]}
          unit={props.suffix}
          disabled={unavailable}
          onValueChange={([low, high], details) => {
            if (pending) {
              details.cancel();
              return;
            }
            sliding.current = true;
            setDraft(formatDraft(low, high));
            setInvalid(false);
          }}
          onValueCommitted={([low, high]) => {
            if (!sliding.current) return;
            sliding.current = false;
            const submitted = formatDraft(low, high);
            setDraft(submitted);
            void commit({min: low, max: high, distribution}, submitted);
          }}
          onCancel={() => {
            if (!sliding.current) return;
            sliding.current = false;
            setDraft(null);
            setInvalid(false);
          }}
        />
        {endpoint("max", maxLabel)}
        {props.suffix && <span>{props.suffix}</span>}
      </div>
      <RandomDistributionSelect value={distribution} disabled={unavailable}
        label={`${label} ${t("分布模式")}`}
        onChange={mode => void commit({...candidate, distribution: mode}, draft)}/>
    </div>
    {invalid && <small id={id + "-error"} className="field-error">{t("请输入有效区间，上限须大于或等于下限")}</small>}
  </div>;
}
