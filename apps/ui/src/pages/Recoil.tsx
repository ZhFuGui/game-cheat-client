import {Suspense} from "react";
import {getGameUiModule} from "~/games/registry";
import {useRuntimeStore} from "~/runtime/state";
export default function RecoilPage(){
  const game=useRuntimeStore(s=>s.workspace?.active?.gameId);
  const Panel=getGameUiModule(game)?.Recoil;
  return Panel?<Suspense fallback={null}><Panel/></Suspense>:<div className="page recoil-page"/>;
}
