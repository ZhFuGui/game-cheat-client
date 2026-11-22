import { clampSidebarWidth, SIDEBAR_MIN_WIDTH, SIDEBAR_TOGGLE_DRAG_DISTANCE } from "@mousebox/shared";

export type SidebarLayout = { width: number; collapsed: boolean };

/** DIMO's two-way drag threshold, anchored again at every fold/unfold transition. */
export function createSidebarDrag(original: SidebarLayout, startX: number) {
  let draft = { ...original };
  let anchorX = startX;
  let baseWidth = original.collapsed ? SIDEBAR_MIN_WIDTH : original.width;
  return (clientX: number): SidebarLayout => {
    if (draft.collapsed) {
      if (clientX - anchorX >= SIDEBAR_TOGGLE_DRAG_DISTANCE) {
        draft = { width: SIDEBAR_MIN_WIDTH, collapsed: false };
        anchorX = clientX;
        baseWidth = SIDEBAR_MIN_WIDTH;
      }
    } else {
      const intendedWidth = baseWidth + clientX - anchorX;
      if (SIDEBAR_MIN_WIDTH - intendedWidth >= SIDEBAR_TOGGLE_DRAG_DISTANCE) {
        draft = { width: SIDEBAR_MIN_WIDTH, collapsed: true };
        anchorX = clientX;
      } else draft = { width: clampSidebarWidth(intendedWidth), collapsed: false };
    }
    return draft;
  };
}
