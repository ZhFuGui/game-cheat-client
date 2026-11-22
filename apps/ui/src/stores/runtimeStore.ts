// Compatibility entry point. State, presentation projection and bridge ownership
// are separate modules so pages do not own subscriptions or native text caches.
export { useRuntimeStore, type RuntimeStore } from "~/runtime/state";
export { connectRuntime, applyAlgorithmDisplay } from "~/runtime/connection";
