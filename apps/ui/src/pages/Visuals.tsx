import { lazy, useMemo } from "react";
import { AppIcon } from "~/components/icons";
import { PageScaffold } from "~/framework/pages/PageScaffold";
import { availableVisualsPages } from "~/navigation/pageGroups";
import {useRuntimeStore} from "~/runtime/state";
import {getGameUiModule} from "~/games/registry";

const views = {general:lazy(()=>import("./VisualsGeneral")), esp: lazy(() => import("./Esp")), radar: lazy(() => import("./Radar")) };
const icons = {
  settings: <AppIcon name="settings" size={14} />,
  esp: <AppIcon name="esp" size={14} />,
  radar: <AppIcon name="radar" size={14} />,
};

export default function Visuals() {
  const game=useRuntimeStore(state=>state.workspace?.active?.gameId);
  const capabilities=useRuntimeStore(state=>state.workspace?.games.find(entry=>entry.id===game)?.capabilities);
  const module=getGameUiModule(game);
  const esp=!!module?.EspSettings&&!!capabilities?.includes("esp");
  const radar=!!module?.Radar&&!!capabilities?.includes("radar");
  const group=useMemo(()=>availableVisualsPages(esp,radar),[esp,radar]);
  return <PageScaffold group={group} views={views} icons={icons} className="visuals-page" />;
}
