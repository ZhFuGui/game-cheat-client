import type {ReactNode} from "react";
import {useTranslation} from "react-i18next";
import {Disclosure} from "./Disclosure";
import {HelpTooltip} from "./HelpTooltip";

/** An unboxed, collapsible group for settings presented directly on the page. */
export function FeatureSection({title, help, children, className = ""}: {
  title: string;
  help?: ReactNode;
  children: ReactNode;
  className?: string;
}) {
  const {t} = useTranslation();
  return <Disclosure label={t(title)} className={`feature-section ${className}`} heading={
    <div className="field-label"><h2>{t(title)}</h2>{help && <HelpTooltip label={title}>{help}</HelpTooltip>}</div>
  }>{children}</Disclosure>;
}
