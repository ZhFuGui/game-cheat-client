import {Suspense} from "react";
import {getGameUiModule} from "~/games/registry";
import {useRuntimeStore} from "~/runtime/state";

/** Shared modules are composed by game metadata, independently from any consuming feature page. */
export default function AimGeneral() {
  const game=useRuntimeStore(s=>s.workspace?.active?.gameId);
  const modules=getGameUiModule(game)?.aimGeneral??[];
  return <div className="page aim-general-page"><div className="aim-general-modules aim-section-stack">
    {modules.map(({id,component:View})=><div key={id} className="aim-general-module" data-aim-module={id}>
      <Suspense fallback={null}><View/></Suspense>
    </div>)}
  </div></div>;
}
