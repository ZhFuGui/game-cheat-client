import {useEffect, useId, useRef, useState, type ReactNode} from "react";
import {useTranslation} from "react-i18next";
import {Input} from "~/components/ui/input";
import {HelpTooltip} from "./HelpTooltip";
import {useSettingBinding, type SettingTarget} from "~/hooks/useSettingBinding";
import {type RandomRange, type RandomDistribution} from "@mousebox/shared";
import {RandomDistributionSelect} from "./RandomDistributionSelect";

export function SettingRangeField(props: SettingTarget & {label: string; help?: ReactNode; suffix?: string}) {
  const {t} = useTranslation();
  const id = useId();
  const {value, validator, save} = useSettingBinding(props);
  const range = value as RandomRange | undefined;
  const min = range?.min;
  const max = range?.max;
  const distribution = range?.distribution ?? "normal";
  const [draft, setDraft] = useState({min: String(min ?? ""), max: String(max ?? "")});
  const [invalid, setInvalid] = useState(false);
  const editing = useRef(false);
  const cancelled = useRef(false);

  useEffect(() => {
    if (!editing.current)
      setDraft({min: String(min ?? ""), max: String(max ?? "")});
  }, [min, max]);

  const commit = (mode = distribution) => {
    editing.current = false;
    if (cancelled.current) {
      cancelled.current = false;
      return;
    }
    const requested = {
      min: draft.min.trim() ? Number(draft.min) : NaN,
      max: draft.max.trim() ? Number(draft.max) : NaN,
      distribution: mode,
    };
    if (!validator?.safeParse(requested).success) {
      setInvalid(true);
      return;
    }
    setInvalid(false);
    if (requested.min !== min || requested.max !== max || requested.distribution !== distribution)
      void save(requested);
  };
  const endpoint = (bound: "min" | "max", label: string) => <Input
    value={draft[bound]}
    type="text"
    inputMode="numeric"
    disabled={min === undefined || max === undefined}
    aria-label={t(props.label) + " " + t(label)}
    aria-invalid={invalid}
    aria-describedby={invalid ? id + "-error" : undefined}
    onChange={e => {
      const field = e.target.value;
      setDraft(current => ({...current, [bound]: field}));
      setInvalid(false);
    }}
    onKeyDown={e => {
      if (e.key === "Escape") {
        cancelled.current = true;
        setDraft({min: String(min ?? ""), max: String(max ?? "")});
        setInvalid(false);
      }
      if (e.key === "Enter" || e.key === "Escape")
        e.currentTarget.blur();
    }}
  />;

  return <div className="field">
    <div className="field-label">
      <span id={id}>{t(props.label)}</span>
      <HelpTooltip label={props.label}>
        {props.help && <p>{props.help}</p>}
        <p>{t("正态分布集中在中点，标准差为区间宽度的六分之一；均匀分布让每个整数等概率；三角分布从中点向两端线性减少；偏向下限和偏向上限分别让较小值和较大值更常见。")}</p>
        <p>{t("所有模式均限制在上下限内，并取整为整数。上下限相同时使用固定值。")}</p>
      </HelpTooltip>
    </div>
    <div className="setting-random-range">
      <div className="setting-range-fields" role="group" aria-labelledby={id}
        onFocus={() => { editing.current = true; }}
        onBlur={e => {
          if (!e.currentTarget.contains(e.relatedTarget))
            commit();
        }}>
        {endpoint("min", "下限")}
        <span aria-hidden="true">–</span>
        {endpoint("max", "上限")}
        <span>{props.suffix}</span>
      </div>
      <RandomDistributionSelect value={distribution} disabled={min === undefined || max === undefined}
        label={t(props.label) + " " + t("分布模式")}
        onChange={(mode: RandomDistribution) => commit(mode)}/>
    </div>
    {invalid && <small id={id + "-error"} className="field-error">{t("请输入有效区间，上限须大于或等于下限")}</small>}
  </div>;
}
