import { useTranslation } from "react-i18next";
import type { ContextCopy } from "./model";
import { usePageContext } from "./usePageContext";

export function PageContextBar() {
  const { t } = useTranslation();
  const { page, active } = usePageContext();
  if (!active) return null;
  const translate = (value: ContextCopy) => t(value.key,
    Object.fromEntries(Object.entries(value.values ?? {}).map(([key, v]) => [
      key, typeof v === "string" ? t(v) : v,
    ])));
  const text = translate(active.text);
  const detail = active.detail ? translate(active.detail) : "";
  return (
    <div className="page-context-bar" aria-label={t("页面上下文栏")}
      data-context-page={page} data-context-id={active.id} data-tone={active.tone}
      title={detail ? text + " · " + detail : text}>
      <span className="context-dot" aria-hidden="true" />
      <div className="context-message" key={active.id}>
        <span className="context-title" role="status" aria-live="polite" aria-atomic="true">{text}</span>
        {detail && <span className="context-detail">{detail}</span>}
      </div>
    </div>
  );
}
