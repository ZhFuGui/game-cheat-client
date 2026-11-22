import {canReorderTab, isMovableTab, type TabDefinition} from "~/framework/tabs/model";
import {moveTab} from "~/framework/tabs/order";

export interface TabPosition extends TabDefinition { left: number; width: number }

/** Fixed regions cannot be entered by either the drag preview or the eventual drop. */
export function tabDragBounds(slots: readonly TabPosition[], id: string): {min: number; max: number} | null {
  const source = slots.find(slot => slot.id === id), movable = slots.filter(isMovableTab);
  if (!source || !canReorderTab(slots, id)) return null;
  const first = movable[0], last = movable[movable.length - 1];
  return {min: first.left - source.left, max: last.left + last.width - source.left - source.width};
}

/** Geometry is measured once per drag; preview transforms never alter these slots. */
export function tabDragOrder(slots: readonly TabPosition[], id: string, offset: number): string[] {
  const index = slots.findIndex(slot => slot.id === id), source = slots[index];
  if (!source || !Number.isFinite(offset)) return slots.map(slot => slot.id);
  let target = index;
  // Compare docking edges so unequal widths can still reach both clamped ends.
  if (offset > 0) {
    const right = source.left + source.width + offset + .01;
    for (let next = index + 1; next < slots.length; next++)
      if (right >= slots[next].left + slots[next].width) target = next;
  } else if (offset < 0) {
    const left = source.left + offset - .01;
    for (let next = index - 1; next >= 0; next--)
      if (left <= slots[next].left) target = next;
  }
  return moveTab(slots, id, target);
}
