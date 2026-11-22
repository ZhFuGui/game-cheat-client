import { useEffect, useRef, useState } from "react";
import { flushSync } from "react-dom";
import type { MouseReportsResult } from "@mousebox/shared";
import {
  nativeSurfaceBounds,
  useNativeSurface,
} from "~/hooks/useNativeSurface";
import { useRuntimeStore } from "~/runtime/state";
import { useCommand } from "~/hooks/useCommand";
import { invoke } from "~/lib/desktopApi";

// Capture owns DOM settlement and the native rectangle acknowledgement together.
// Keep this ordered transaction separate from summary/chart presentation.
export function useMouseReportCapture() {
  const ref = useNativeSurface("reports"),
    { run, pending } = useCommand();
  const state = useRuntimeStore((s) => s.mouseReports),
    learningBusy = useRuntimeStore(
      (s) =>
        s.learning?.recording || s.learning?.training || s.learning?.ioBusy,
    );
  const [result, setResult] = useState<MouseReportsResult | null>(null),
    [readError, setReadError] = useState(""),
    [readAttempt, setReadAttempt] = useState(0);
  const [preparing, setPreparing] = useState(false),
    [preparedRevision, setPreparedRevision] = useState<string | null>(null);
  const mounted = useRef(true);
  const recording = state?.recording ?? false,
    busy =
      preparing ||
      recording ||
      state?.analyzing ||
      state?.busy ||
      learningBusy ||
      pending;
  const start = async () => {
    const area = ref.current;
    if (!state || busy || !area) return;
    // Settle result removal and any scroll clamping before native capture owns the rectangle.
    flushSync(() => {
      setPreparing(true);
      setPreparedRevision(state.revision);
    });
    let started = false;
    try {
      if (
        !(await run("surface.update", nativeSurfaceBounds("reports", area))) ||
        !mounted.current
      )
        return;
      started = !!(await run("mouseReports.start", {}));
    } finally {
      if (mounted.current) {
        if (!started) setPreparedRevision(null);
        setPreparing(false);
      }
    }
  };
  useEffect(() => {
    let alive = true;
    const context = useRuntimeStore.getState().workspace?.active;
    if (!state?.hasResult || recording || state.analyzing) return;
    void invoke("mouseReports.read", {}, context)
      .then((value) => {
        if (alive && value.result?.reportId === state.revision) {
          setResult(value.result);
          setReadError("");
        }
      })
      .catch((error) => {
        if (alive) setReadError(String(error));
      });
    return () => {
      alive = false;
    };
  }, [
    state?.revision,
    state?.hasResult,
    state?.analyzing,
    recording,
    readAttempt,
  ]);
  useEffect(() => {
    mounted.current = true;
    const context = useRuntimeStore.getState().workspace?.active;
    const escape = (event: KeyboardEvent) => {
      if (
        event.key === "Escape" &&
        useRuntimeStore.getState().mouseReports?.recording
      ) {
        event.preventDefault();
        void invoke("mouseReports.stop", {}, context).catch(() => {});
      }
    };
    window.addEventListener("keydown", escape);
    return () => {
      mounted.current = false;
      window.removeEventListener("keydown", escape);
      void invoke("mouseReports.stop", {}, context).catch(() => {});
    };
  }, []);
  const exportRaw = async () => {
    const file = await run("file.save", { purpose: "mouse-reports" });
    if (file?.path) await run("mouseReports.export", { path: file.path });
  };
  const prepared = preparedRevision === state?.revision;
  const visibleResult =
    state?.hasResult &&
    !preparing &&
    !prepared &&
    !recording &&
    !state.analyzing &&
    result?.reportId === state.revision
      ? result
      : null;
  return {
    ref,
    state,
    recording,
    busy,
    preparing,
    prepared,
    visibleResult,
    readError,
    start,
    stop: () => run("mouseReports.stop", {}),
    clear: () => run("mouseReports.clear", {}),
    retryRead: () => setReadAttempt((n) => n + 1),
    exportRaw,
  };
}
