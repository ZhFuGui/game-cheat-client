import {Suspense} from "react";
import {getGameUiModule} from "~/games/registry";
import {useRuntimeStore} from "~/runtime/state";

export default function VisualsGeneral(){
  const game=useRuntimeStore(s=>s.workspace?.active?.gameId);
  const Settings=getGameUiModule(game)?.VisualsSettings;
  return Settings?<Suspense fallback={null}><Settings/></Suspense>:null;
}
