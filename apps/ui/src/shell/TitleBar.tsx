import { AppIcon } from "~/components/icons";
import { useTranslation } from "react-i18next";
import { useCommand } from "~/hooks/useCommand";
import { PageContextBar } from "~/features/page-context/PageContextBar";
export function TitleBar() {
  const { t } = useTranslation(),
    { run } = useCommand();
  return (
    <div className="titlebar drag-region">
      <PageContextBar />
      <div className="window-actions no-drag">
        <button
          aria-label={t("最小化")}
          onClick={() => void run("window.minimize", {})}
        >
          <AppIcon name="minimize" size={14} />
        </button>
        <button
          aria-label={t("最大化或还原")}
          onClick={() => void run("window.maximize", {})}
        >
          <AppIcon name="maximize" size={12} />
        </button>
        <button
          className="close-window"
          aria-label={t("关闭")}
          onClick={() => void run("window.close", {})}
        >
          <AppIcon name="close" size={16} />
        </button>
      </div>
    </div>
  );
}
