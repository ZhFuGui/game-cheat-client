import { clampSidebarWidth, SIDEBAR_COLLAPSED_WIDTH } from "@mousebox/shared";
import type { SidebarLayout } from "./sidebarDrag";

/** Keep pointer-frequency layout outside React and the desktop bridge. */
export function applySidebarLayout(layout: SidebarLayout): SidebarLayout {
  const next = { width: clampSidebarWidth(layout.width), collapsed: layout.collapsed };
  document.documentElement.style.setProperty("--sidebar-width", `${next.collapsed ? SIDEBAR_COLLAPSED_WIDTH : next.width}px`);
  document.documentElement.dataset.sidebarCollapsed = String(next.collapsed);
  return next;
}
