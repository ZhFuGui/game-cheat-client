import { useEffect, useState } from "react";
import { sameWorkspaceContext, type Cs2RadarFrame } from "@mousebox/shared";
import { invoke } from "~/lib/desktopApi";
import { useRuntimeStore } from "~/runtime/state";

const frameIntervalMs = 1000 / 30;
const idleIntervalMs = 180;

/** Pull only the latest accepted projection while this tab is mounted and the window is visible. */
export function useRadarFrame() {
  const context = useRuntimeStore(state => state.workspace?.active);
  const connected = useRuntimeStore(state => state.connected);
  const refreshing = useRuntimeStore(state => state.workspaceRefreshing);
  const [frame, setFrame] = useState<Cs2RadarFrame | null>(null);
  const [failed, setFailed] = useState(false);
  const enabled = connected && !refreshing && context?.gameId === "cs2";

  useEffect(() => {
    setFrame(null);
    setFailed(false);
    if (!enabled || !context) return;
    let stopped = false, reading = false, generation = 0;
    let after: string | undefined;
    let timer: ReturnType<typeof setTimeout> | undefined;

    const poll = async () => {
      if (stopped || reading || document.hidden) return;
      reading = true;
      const request = generation;
      const started = performance.now();
      let interval = frameIntervalMs;
      const current = () => !stopped && !document.hidden && generation === request &&
        sameWorkspaceContext(context, useRuntimeStore.getState().workspace?.active);
      try {
        const result = await invoke("game.cs2.radar.read", after ? { after } : {}, context);
        if (!current()) return;
        if (!result.unchanged) {
          after = result.frame?.revision;
          setFrame(result.frame);
        }
        if (!after) interval = idleIntervalMs;
        setFailed(false);
      } catch {
        if (!current()) return;
        after = undefined;
        setFrame(null);
        setFailed(true);
        interval = 600;
      } finally {
        reading = false;
        if (!stopped && !document.hidden)
          timer = setTimeout(() => void poll(), Math.max(0, interval - (performance.now() - started)));
      }
    };

    const visibility = () => {
      clearTimeout(timer);
      if (document.hidden) {
        ++generation;
        after = undefined;
        setFrame(null);
        setFailed(false);
      } else {
        void poll();
      }
    };
    document.addEventListener("visibilitychange", visibility);
    void poll();
    return () => {
      stopped = true;
      clearTimeout(timer);
      document.removeEventListener("visibilitychange", visibility);
    };
  }, [enabled, context]);

  return { frame: enabled ? frame : null, failed: enabled && failed };
}
