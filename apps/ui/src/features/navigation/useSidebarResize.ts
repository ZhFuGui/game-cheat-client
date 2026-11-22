import { useEffect, useId, useLayoutEffect, useRef, useState, type KeyboardEvent, type PointerEvent } from "react";
import { clampSidebarWidth, SIDEBAR_DEFAULT_WIDTH, SIDEBAR_MIN_WIDTH, SIDEBAR_MAX_WIDTH, SIDEBAR_COLLAPSED_WIDTH, type UiPreferences } from "@mousebox/shared";
import { useRuntimeStore } from "~/runtime/state";
import { useSurfaceStore } from "~/stores/surfaceStore";
import { useCommand } from "~/hooks/useCommand";
import { applySidebarLayout } from "./sidebarLayout";
import { createSidebarDrag, type SidebarLayout } from "./sidebarDrag";

/** Shell-only interaction. CSS previews every frame; global preferences save once on commit. */
export function useSidebarResize() {
  const storedWidth = useRuntimeStore(state => state.preferences.sidebarWidth ?? SIDEBAR_DEFAULT_WIDTH);
  const storedCollapsed = useRuntimeStore(state => state.preferences.sidebarCollapsed ?? false);
  const [layout, setLayout] = useState(() => ({ width: clampSidebarWidth(storedWidth), collapsed: storedCollapsed }));
  const [isResizing, setIsResizing] = useState(false);
  const layoutRef = useRef(layout);
  const cleanupRef = useRef<((commit: boolean) => void) | null>(null);
  const mounted = useRef(true);
  const saveRevision = useRef(0);
  const pendingSave = useRef<number | null>(null);
  const { run } = useCommand();
  const command = useRef(run);
  const owner = `sidebar-resize:${useId()}`;

  useLayoutEffect(() => { command.current = run; }, [run]);
  useLayoutEffect(() => {
    // A late preference event must not replace the pointer's draft or a newer save.
    if (cleanupRef.current || pendingSave.current !== null) return;
    layoutRef.current = applySidebarLayout({ width: storedWidth, collapsed: storedCollapsed });
    setLayout(layoutRef.current);
  }, [storedWidth, storedCollapsed]);
  useEffect(() => {
    mounted.current = true;
    return () => { mounted.current = false; cleanupRef.current?.(false); };
  }, []);

  const commitLayout = (requested: SidebarLayout) => {
    const next = applySidebarLayout(requested), previous = layoutRef.current;
    const patch: Partial<UiPreferences> = {};
    if (next.width !== previous.width) patch.sidebarWidth = next.width;
    if (next.collapsed !== previous.collapsed) patch.sidebarCollapsed = next.collapsed;
    if (!Object.keys(patch).length) return;
    layoutRef.current = next;
    if (mounted.current) setLayout(next);
    const revision = ++saveRevision.current;
    pendingSave.current = revision;
    void command.current("preferences.set", patch).then(result => {
      if (pendingSave.current !== revision) return;
      pendingSave.current = null;
      if (!result && mounted.current && !cleanupRef.current) {
        const stored = useRuntimeStore.getState().preferences;
        layoutRef.current = applySidebarLayout({ width: stored.sidebarWidth ?? SIDEBAR_DEFAULT_WIDTH, collapsed: stored.sidebarCollapsed ?? false });
        setLayout(layoutRef.current);
      }
    });
  };

  const startResize = (event: PointerEvent<HTMLElement>) => {
    if (event.button !== 0 || cleanupRef.current) return;
    event.preventDefault();
    event.stopPropagation();
    const handle = event.currentTarget, pointer = event.pointerId;
    const original = layoutRef.current;
    const drag = createSidebarDrag(original, event.clientX);
    const root = document.documentElement;
    const previousCursor = document.body.style.cursor;
    const previousSelection = document.body.style.userSelect;
    let draft = original, frame: number | null = null, active = true;

    const finish = (commit: boolean) => {
      if (!active) return;
      active = false;
      if (frame !== null) cancelAnimationFrame(frame);
      window.removeEventListener("pointermove", move);
      window.removeEventListener("pointerup", up);
      window.removeEventListener("pointercancel", cancel);
      window.removeEventListener("blur", blur);
      window.removeEventListener("keydown", key);
      handle.removeEventListener("lostpointercapture", lostCapture);
      cleanupRef.current = null;
      if (commit) commitLayout(draft);
      else applySidebarLayout(original);
      document.body.style.cursor = previousCursor;
      document.body.style.userSelect = previousSelection;
      root.classList.remove("is-sidebar-resizing");
      useSurfaceStore.getState().setOccluded(false, owner);
      try { if (handle.hasPointerCapture(pointer)) handle.releasePointerCapture(pointer); } catch { /* Window may be gone. */ }
      if (mounted.current) setIsResizing(false);
    };
    const move = (e: globalThis.PointerEvent) => {
      if (e.pointerId !== pointer) return;
      if (!(e.buttons & 1)) { finish(true); return; }
      draft = drag(e.clientX);
      if (frame !== null) return;
      frame = requestAnimationFrame(() => { frame = null; applySidebarLayout(draft); });
    };
    const up = (e: globalThis.PointerEvent) => {
      if (e.pointerId !== pointer) return;
      draft = drag(e.clientX);
      finish(true);
    };
    const cancel = (e: globalThis.PointerEvent) => { if (e.pointerId === pointer) finish(false); };
    const blur = () => finish(true);
    const lostCapture = () => finish(true);
    const key = (e: globalThis.KeyboardEvent) => { if (e.key === "Escape") { e.preventDefault(); finish(false); } };

    cleanupRef.current = finish;
    document.body.style.cursor = "col-resize";
    document.body.style.userSelect = "none";
    root.classList.add("is-sidebar-resizing");
    // A Win32 child canvas must not intercept the drag when the rail crosses it.
    useSurfaceStore.getState().setOccluded(true, owner);
    setIsResizing(true);
    window.addEventListener("pointermove", move);
    window.addEventListener("pointerup", up);
    window.addEventListener("pointercancel", cancel);
    window.addEventListener("blur", blur);
    window.addEventListener("keydown", key);
    handle.addEventListener("lostpointercapture", lostCapture);
    try { handle.setPointerCapture(pointer); } catch { /* Global listeners still cover this document. */ }
  };
  const onKeyDown = (event: KeyboardEvent<HTMLElement>) => {
    if (cleanupRef.current) return;
    const step = event.shiftKey ? 10 : 1;
    const current = layoutRef.current;
    const next = event.key === "ArrowLeft" ? { width: current.width - (current.collapsed ? 0 : step), collapsed: current.collapsed || current.width <= SIDEBAR_MIN_WIDTH }
      : event.key === "ArrowRight" ? { width: current.collapsed ? SIDEBAR_MIN_WIDTH : current.width + step, collapsed: false }
      : event.key === "Home" ? { width: SIDEBAR_MIN_WIDTH, collapsed: false }
      : event.key === "End" ? { width: SIDEBAR_MAX_WIDTH, collapsed: false }
      : event.key === "Enter" ? { width: SIDEBAR_DEFAULT_WIDTH, collapsed: false } : null;
    if (next === null) return;
    event.preventDefault();
    commitLayout(next);
  };
  return { width: layout.collapsed ? SIDEBAR_COLLAPSED_WIDTH : layout.width, collapsed: layout.collapsed, isResizing, startResize, onKeyDown,
    resetWidth: () => commitLayout({ width: SIDEBAR_DEFAULT_WIDTH, collapsed: false }) };
}
