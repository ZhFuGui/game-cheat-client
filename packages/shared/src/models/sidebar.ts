// Expanded width limits shared by resizing and preference validation.
export const SIDEBAR_DEFAULT_WIDTH = 175;
export const SIDEBAR_MIN_WIDTH = 150;
export const SIDEBAR_MAX_WIDTH = 240;
export const SIDEBAR_COLLAPSED_WIDTH = 56;
export const SIDEBAR_TOGGLE_DRAG_DISTANCE = 150;

export function clampSidebarWidth(value: number): number {
  return Number.isFinite(value)
    ? Math.max(SIDEBAR_MIN_WIDTH, Math.min(SIDEBAR_MAX_WIDTH, Math.round(value)))
    : SIDEBAR_DEFAULT_WIDTH;
}
