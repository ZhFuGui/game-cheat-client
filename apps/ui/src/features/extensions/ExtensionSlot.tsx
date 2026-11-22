import { Suspense } from "react";
import { getGameUiModule } from "~/games/registry";
import type { GameExtensionSlot } from "~/games/contracts";
import { useRuntimeStore } from "~/runtime/state";

/** Each game contributes lazy panels; the common page has no plugin switch. */
export function ExtensionSlot({ name }: { name: GameExtensionSlot }) {
  const game = useRuntimeStore(state => state.workspace?.active?.gameId);
  return <>{getGameUiModule(game)?.extensions?.map(extension => {
    const Panel = extension.slots[name];
    return Panel ? <Suspense key={extension.id} fallback={null}><Panel /></Suspense> : null;
  })}</>;
}
