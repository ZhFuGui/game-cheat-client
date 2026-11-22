import { useEffect, useRef } from "react";
import type { CommandInput } from "@mousebox/shared";
import { invoke } from "~/lib/desktopApi";
import { useSurfaceStore } from "~/stores/surfaceStore";
import { useRuntimeStore } from "~/runtime/state";
type SurfaceKind = CommandInput<"surface.update">["kind"];
export function nativeSurfaceBounds(kind: SurfaceKind, element: HTMLElement): CommandInput<"surface.update"> {
  const r = element.getBoundingClientRect();
  const contained = kind !== "reports" || (r.x >= 0 && r.y >= 0 && r.right <= window.innerWidth + 1 && r.bottom <= window.innerHeight + 1);
  return {kind, x:contained?r.x:0, y:contained?r.y:0, width:contained?r.width:0, height:contained?r.height:0,
    scale:window.devicePixelRatio||1, active:contained&&r.width>0&&r.height>0,
    visible:contained&&document.visibilityState!=="hidden"&&r.width>0&&r.height>0,
    occluded:useSurfaceStore.getState().occluded};
}
export function useNativeSurface(kind: SurfaceKind) {
  const ref = useRef<HTMLDivElement>(null);
  const occluded = useSurfaceStore((s) => s.occluded);
  const context = useRuntimeStore((s) => s.workspace?.active);
  useEffect(() => {
    const element = ref.current;
    if (!element) return;
    let previous = "";
    let active = true;
    let last: CommandInput<"surface.update"> | undefined;
    const update = () => {
      if (!active) return;
      const value = { ...nativeSurfaceBounds(kind, element), occluded };
      const signature = JSON.stringify(value);
      if (signature === previous) return;
      previous = signature;
      last = value;
      void invoke("surface.update", value, context).catch(() => {});
    };
    const observer = new ResizeObserver(update);
    observer.observe(element);
    window.addEventListener("resize", update);
    window.addEventListener("scroll", update, true);
    document.addEventListener("visibilitychange", update);
    update();
    return () => {
      active = false;
      observer.disconnect();
      window.removeEventListener("resize", update);
      window.removeEventListener("scroll", update, true);
      document.removeEventListener("visibilitychange", update);
      if (last)
        void invoke("surface.update", { ...last, visible: false,active:false }, context).catch(
          () => {},
        );
    };
  }, [kind, occluded, context]);
  return ref;
}
