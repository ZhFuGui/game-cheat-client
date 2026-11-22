import { useEffect, useId, useRef, type MouseEvent, type PointerEvent, type RefObject } from "react";
import {canReorderTab, type TabDefinition} from "~/framework/tabs/model";
import { tabDragBounds, tabDragOrder, type TabPosition } from "./geometry";

interface TabReorderOptions {
  list: RefObject<HTMLDivElement | null>;
  items: readonly TabDefinition[];
  onReorder?: (ids: string[]) => void;
  onDragChange?: (active: boolean, owner: string) => void;
}
/** Pointer preview owns transforms only. Routing, persistence and selected content are untouched. */
export function useTabReorder({ list, items, onReorder, onDragChange }: TabReorderOptions) {
  const cleanup = useRef<((commit: boolean) => void) | null>(null);
  const suppressClick = useRef(false), owner = `page-tabs:${useId()}`;
  const ids = items.map(item => item.id), enabled = !!onReorder;
  const signature = JSON.stringify(items.map(item => [item.id, item.placement]));
  useEffect(() => () => cleanup.current?.(false), [signature, enabled]);

  const start = (event: PointerEvent<HTMLButtonElement>, id: string) => {
    suppressClick.current = false;
    if (event.button !== 0 || !onReorder || !canReorderTab(items,id) || cleanup.current) return;
    const root = list.current, handle = event.currentTarget;
    const track = root?.querySelector<HTMLDivElement>(".page-tabs-track");
    if (!root || !track) return;
    const elements = [...track.querySelectorAll<HTMLButtonElement>('[role="tab"]')];
    const surfaces = elements.map(element => element.closest<HTMLElement>(".page-tab-item") ?? element);
    const surface = surfaces[elements.indexOf(handle)];
    const slots: TabPosition[] = elements.map(element => {
      const rect = element.getBoundingClientRect();
      const item = items.find(item => item.id === element.dataset.tabId)!;
      return { id: item.id, placement: item.placement, left: rect.left, width: rect.width };
    });
    const source = slots.find(slot => slot.id === id);
    const bounds = tabDragBounds(slots,id);
    if (!source || !source.width || !bounds) return;
    event.preventDefault();
    handle.focus({ preventScroll: true });
    const pointer = event.pointerId, startX = event.clientX, startY = event.clientY, startScroll = track.scrollLeft;
    const oldCursor = document.body.style.cursor, oldSelection = document.body.style.userSelect;
    const styles = surfaces.map(element => ({ transform: element.style.transform, zIndex: element.style.zIndex }));
    const gap = slots.length > 1 ? slots[1].left - slots[0].left - slots[0].width : 0;
    const {min,max} = bounds;
    let active = true, dragging = false, frame: number | null = null, clientX = startX;
    let draft = [...ids];
    const offset = () => Math.max(min, Math.min(max, clientX - startX + track.scrollLeft - startScroll));
    const preview = () => {
      frame = null;
      if (!active || !dragging) return;
      const bounds = track.getBoundingClientRect(), before = track.scrollLeft;
      if (clientX < bounds.left + 24) track.scrollLeft -= 12;
      else if (clientX > bounds.right - 24) track.scrollLeft += 12;
      const delta = offset();
      draft = tabDragOrder(slots, id, delta);
      let left = slots[0].left;
      const positions = new Map<string, number>();
      for (const current of draft) { positions.set(current, left); left += slots.find(slot => slot.id === current)!.width + gap; }
      surfaces.forEach((element, index) => {
        element.style.transform = `translateX(${slots[index].id === id ? delta : positions.get(slots[index].id)! - slots[index].left}px)`;
      });
      if (track.scrollLeft !== before) frame = requestAnimationFrame(preview);
    };
    const finish = (commit: boolean) => {
      if (!active) return;
      active = false;
      if (frame !== null) cancelAnimationFrame(frame);
      window.removeEventListener("pointermove", move);
      window.removeEventListener("pointerup", up);
      window.removeEventListener("pointercancel", cancel);
      window.removeEventListener("keydown", key);
      window.removeEventListener("blur", blur);
      handle.removeEventListener("lostpointercapture", lost);
      cleanup.current = null;
      if (dragging) {
        suppressClick.current = true;
        document.body.style.cursor = oldCursor;
        document.body.style.userSelect = oldSelection;
        delete root.dataset.reordering;
        delete handle.dataset.dragging;
        delete surface.dataset.dragging;
        surfaces.forEach((element, index) => { element.style.transform = styles[index].transform; element.style.zIndex = styles[index].zIndex; });
        onDragChange?.(false, owner);
      }
      try { if (handle.hasPointerCapture(pointer)) handle.releasePointerCapture(pointer); } catch { /* Closing window. */ }
      if (commit && dragging && draft.some((value, index) => value !== ids[index])) onReorder(draft);
    };
    const move = (e: globalThis.PointerEvent) => {
      if (e.pointerId !== pointer) return;
      if (!(e.buttons & 1)) { finish(false); return; }
      clientX = e.clientX;
      if (!dragging) {
        if (Math.abs(clientX - startX) < 6 || Math.abs(clientX - startX) <= Math.abs(e.clientY - startY)) return;
        dragging = true;
        root.dataset.reordering = "true"; handle.dataset.dragging = "true";
        surface.dataset.dragging = "true"; surface.style.zIndex = "3";
        document.body.style.cursor = "grabbing"; document.body.style.userSelect = "none";
        onDragChange?.(true, owner);
      }
      e.preventDefault();
      if (frame === null) frame = requestAnimationFrame(preview);
    };
    const up = (e: globalThis.PointerEvent) => {
      if (e.pointerId !== pointer) return;
      clientX = e.clientX;
      if (dragging) draft = tabDragOrder(slots, id, offset());
      finish(true);
    };
    const cancel = (e: globalThis.PointerEvent) => { if (e.pointerId === pointer) finish(false); };
    const key = (e: globalThis.KeyboardEvent) => { if (e.key === "Escape") { e.preventDefault(); finish(false); } };
    const blur = () => finish(false), lost = () => finish(false);
    cleanup.current = finish;
    window.addEventListener("pointermove", move, { passive: false });
    window.addEventListener("pointerup", up);
    window.addEventListener("pointercancel", cancel);
    window.addEventListener("keydown", key);
    window.addEventListener("blur", blur);
    handle.addEventListener("lostpointercapture", lost);
    try { handle.setPointerCapture(pointer); } catch { /* Global listeners remain usable. */ }
  };
  return { start, consumeClick: (event: MouseEvent) => {
    if (!suppressClick.current || event.detail === 0) return false;
    suppressClick.current = false; event.preventDefault(); event.stopPropagation(); return true;
  } };
}
