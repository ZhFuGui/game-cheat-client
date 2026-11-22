import { useEffect, useState } from "react";
import { useRuntimeStore } from "~/runtime/state";
import { useCommand } from "~/hooks/useCommand";
import { invoke } from "~/lib/desktopApi";

// Owns capture lifecycle, validation, file workflows and destructive actions.
// Views receive explicit actions and cannot issue arbitrary native commands.
export function useLearningSession() {
  const { run, pending } = useCommand(),
    learning = useRuntimeStore((s) => s.learning),
    reportCaptureBusy = useRuntimeStore(
      (s) => s.mouseReports?.recording || s.mouseReports?.analyzing,
    ),
    [dpi, setDpi] = useState(String(learning?.dpi ?? "")),
    [dpiError, setDpiError] = useState(false),
    [confirmation, setConfirmation] = useState<"clear" | "delete" | null>(null),
    [deletion, setDeletion] = useState<{ path: string; token: string } | null>(
      null,
    );
  const recording = learning?.recording ?? false,
    busy =
      learning?.training || learning?.ioBusy || recording || reportCaptureBusy;
  useEffect(() => {
    if (learning?.dpi !== undefined) setDpi(String(learning.dpi));
  }, [learning?.dpi]);
  useEffect(() => {
    const context = useRuntimeStore.getState().workspace?.active;
    const key = (e: KeyboardEvent) => {
      if (
        e.key === "Escape" &&
        useRuntimeStore.getState().learning?.recording
      ) {
        e.preventDefault();
        void invoke("learning.pause", {}, context).catch(() => {});
      }
    };
    window.addEventListener("keydown", key);
    return () => {
      window.removeEventListener("keydown", key);
      if (useRuntimeStore.getState().learning?.recording)
        void invoke("learning.pause", {}, context).catch(() => {});
    };
  }, []);
  const parsedDpi = () => {
    const value = Number(dpi);
    if (
      !dpi.trim() ||
      !Number.isInteger(value) ||
      value < 100 ||
      value > 32000
    ) {
      setDpiError(true);
      return null;
    }
    setDpiError(false);
    return value;
  };
  const start = async () => {
    const value = parsedDpi();
    if (value !== null) await run("learning.start", { dpi: value });
  };
  const openSamples = async () => {
    const result = await run("file.open", {
      purpose: "samples",
      multiple: true,
    });
    if (result?.paths.length)
      await run("samples.import", { paths: result.paths });
  };
  const openModel = async () => {
    const result = await run("file.open", { purpose: "model" });
    if (result?.paths[0]) await run("model.import", { path: result.paths[0] });
  };
  const save = async (purpose: "samples" | "model") => {
    const result = await run("file.save", { purpose });
    if (result?.path)
      await run(purpose === "samples" ? "samples.export" : "model.export", {
        path: result.path,
      });
  };
  const requestDelete = async () => {
    const result = await run("model.requestDelete", {});
    if (result) {
      setDeletion(result);
      setConfirmation("delete");
    }
  };

  const commitDpi = () => {
    const value = parsedDpi();
    if (value !== null && value !== learning?.dpi)
      void run("learning.setDpi", { dpi: value });
  };
  const confirm = async () => {
    const result = await (confirmation === "delete" && deletion
      ? run("model.confirmDelete", { token: deletion.token })
      : run("learning.clear", {}));
    if (result) setConfirmation(null);
  };
  return {
    learning,
    busy,
    recording,
    pending,
    dpi,
    dpiError,
    confirmation,
    deletion,
    changeDpi: (value: string) => {
      setDpi(value);
      setDpiError(false);
    },
    commitDpi,
    start,
    openSamples,
    openModel,
    save,
    requestDelete,
    confirm,
    requestClear: () => setConfirmation("clear"),
    dismissConfirmation: () => setConfirmation(null),
    pause: () => run("learning.pause", {}),
    replay: () => run("learning.replay", {}),
    fit: () => run("learning.fit", {}),
    toggleActive: () => run("model.setActive", { active: !learning?.active }),
    setVariation: (variation: boolean) =>
      run("learning.setVariation", { variation }),
  };
}

export type LearningSession = ReturnType<typeof useLearningSession>;
