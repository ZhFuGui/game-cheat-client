import { sameWorkspaceContext, type EventPayload, type Topic } from "@mousebox/shared";
import { invoke, subscribe } from "~/lib/desktopApi";
import { useRuntimeStore } from "./state";
import { applyBootstrap, applyWorkspace, listenRuntimeEvents } from "./events";

export { applyAlgorithmDisplay } from "./events";

let stopEvents: (() => void) | undefined;
let connectionGeneration = 0;

export async function connectRuntime() {
  const generation = ++connectionGeneration;
  let catalogPushed = false;
  stopEvents?.();
  if (!window.mousebox) {
    useRuntimeStore.setState({ connected: false, error: "Desktop bridge is unavailable" });
    return () => {};
  }

  const stops: (() => void)[] = [];
  const listen = <K extends Topic>(topic: K, handle: (payload: EventPayload<K>) => void) => {
    stops.push(subscribe(topic, handle));
  };
  const stop = () => {
    for (const unsubscribe of stops.splice(0)) unsubscribe();
    if (stopEvents === stop) stopEvents = undefined;
  };
  try {
    listen("workspace", catalog => {
      catalogPushed = true;
      applyWorkspace(catalog);
    });
    listenRuntimeEvents(listen);
    stopEvents = stop;
    const boot = await invoke("bootstrap", {});
    if (generation === connectionGeneration) {
      const current = useRuntimeStore.getState().workspace;
      const bothInactive = current?.active === null && boot.workspace?.active === null;
      // An event may already have advanced past the catalog in the reply.
      if (catalogPushed && current && boot.workspace && !bothInactive &&
          !sameWorkspaceContext(current.active, boot.workspace.active)) return stop;
      applyBootstrap(boot);
    }
  } catch (error) {
    stop();
    if (generation === connectionGeneration) {
      useRuntimeStore.getState().setError(error instanceof Error ? error.message : String(error));
      useRuntimeStore.setState({ connected: false });
    }
  }
  return stop;
}
