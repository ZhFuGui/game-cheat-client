import {Suspense} from "react";
import {getGameUiModule} from "~/games/registry";
import {useRuntimeStore} from "~/runtime/state";
import {ReaderChain} from "~/features/reader-metrics/ReaderChain";
export function ReaderConnection(){
  const gameId=useRuntimeStore(s=>s.workspace?.active?.gameId);
  const SourcePanel=getGameUiModule(gameId)?.SourcePanel;
  return <div className="reader-connection-panel">
    <Suspense fallback={null}>{SourcePanel&&<SourcePanel/>}</Suspense>
    <ReaderChain/>
  </div>;
}
