import {useId, useState, type ReactNode} from "react";
import {useTranslation} from "react-i18next";
import {AppIcon} from "~/components/icons/AppIcon";

/** The disclosure button is separate from setting switches, labels and help buttons. */
export function Disclosure({label, heading, children, className = ""}: {
  label: string;
  heading: ReactNode;
  children: ReactNode;
  className?: string;
}) {
  const {t} = useTranslation();
  const [expanded, setExpanded] = useState(true);
  const panelId = useId();
  return <div className={`disclosure ${className}`} role="group" aria-label={label}>
    <div className="disclosure-heading">
      <button type="button" className="disclosure-trigger" aria-expanded={expanded}
        aria-controls={panelId} aria-label={`${label} ${t(expanded ? "收起" : "展开")}`}
        onClick={() => setExpanded(value => !value)}>
        <AppIcon name={expanded ? "chevronDown" : "chevronRight"} size={16}/>
      </button>
      <div className="disclosure-title">{heading}</div>
    </div>
    <div id={panelId} className="disclosure-content" hidden={!expanded}>{children}</div>
  </div>;
}
