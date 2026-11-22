import { useTranslation } from "react-i18next";
import { SIDEBAR_COLLAPSED_WIDTH, SIDEBAR_MAX_WIDTH } from "@mousebox/shared";
import { useSidebarResize } from "./useSidebarResize";

export function SidebarResizeHandle() {
  const { t } = useTranslation();
  const resize = useSidebarResize();
  return <div className="sidebar-resize-handle no-drag" role="separator" tabIndex={0}
    aria-label={t("调整侧栏宽度")} aria-orientation="vertical" aria-controls="primary-sidebar"
    aria-valuemin={SIDEBAR_COLLAPSED_WIDTH} aria-valuemax={SIDEBAR_MAX_WIDTH} aria-valuenow={resize.width}
    aria-valuetext={resize.collapsed ? t("侧栏已收起") : `${resize.width}px`}
    title={t("向左拖动可收起，向右拖动展开，双击恢复默认")}
    data-resizing={resize.isResizing || undefined}
    onPointerDown={resize.startResize} onKeyDown={resize.onKeyDown} onDoubleClick={resize.resetWidth} />;
}
