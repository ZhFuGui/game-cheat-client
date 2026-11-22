import {Suspense} from "react";
import { getGameUiModule } from "~/games/registry";
import { useRuntimeStore } from "~/runtime/state";

export default function GameDataView() {
  const gameId = useRuntimeStore(s => s.workspace?.active?.gameId);
  const DataPanel = getGameUiModule(gameId)?.DataPanel;
  return <Suspense fallback={null}>{DataPanel&&<DataPanel/>}</Suspense>;
}
