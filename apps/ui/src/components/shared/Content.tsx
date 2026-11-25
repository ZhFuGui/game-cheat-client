import type { ReactNode } from "react";
import { useTranslation } from "react-i18next";
import {HelpTooltip} from "./HelpTooltip";
import { AppIcon, type AppIconName } from "~/components/icons";
export function Section({
  title,
  description,
  help,
  children,
  className = "",
}: {
  title?: string;
  description?: string;
  help?: ReactNode;
  children: ReactNode;
  className?: string;
}) {
  const { t } = useTranslation();
  return (
    <section className={`section ${className}`}>
      {title && (
        <div className="section-heading">
          <div className="field-label"><h2>{t(title)}</h2>{(help||description)&&<HelpTooltip label={title}>{description&&<p>{t(description)}</p>}{help}</HelpTooltip>}</div>
        </div>
      )}
      {children}
    </section>
  );
}
export function Hint({ children }: { children: ReactNode }) {
  return <p className="hint">{children}</p>;
}
export function EmptyState({ icon, message, action, className = "" }: {
  icon: AppIconName;
  message: string;
  action?: ReactNode;
  className?: string;
}) {
  return (
    <div className={`feedback-state ${className}`}>
      <span className="feedback-state-icon"><AppIcon name={icon} size={21} /></span>
      <p>{message}</p>
      {action}
    </div>
  );
}
export function StateText({ field }: { field: string }) {
  return <span>{field || "—"}</span>;
}
export const display = (value: unknown): string =>
  value === null || value === undefined || value === "" ? "—" : String(value);
