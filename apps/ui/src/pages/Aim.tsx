import {lazy,useMemo} from "react";
import {Navigate,useSearchParams} from "react-router";
import {AppIcon} from "~/components/icons";
import {PageScaffold} from "~/framework/pages/PageScaffold";
import {NavigationHelp} from "~/features/navigation/NavigationHelp";
import {aimPages} from "~/navigation/pageGroups";
import {getGameUiModule} from "~/games/registry";
import {useRuntimeStore} from "~/runtime/state";

const views={general:lazy(()=>import("./AimGeneral")),aimbot:lazy(()=>import("./Aimbot")),triggerbot:lazy(()=>import("./Triggerbot")),rage:lazy(()=>import("./Rage"))};
const icons={settings:<AppIcon name="settings" size={14}/>,aimbot:<AppIcon name="aimbot" size={14}/>,triggerbot:<AppIcon name="triggerbot" size={14}/>};
export default function Aim(){
  const game=useRuntimeStore(s=>s.workspace?.active?.gameId);
  const module=getGameUiModule(game);
  const rageSupported=useRuntimeStore(s=>s.workspace?.games.find(item=>item.id===game)?.capabilities.includes("rage")??false);
  const hasRage=!!module?.Rage&&rageSupported;
  const [params]=useSearchParams();
  const group=useMemo(()=>({...aimPages,sections:aimPages.sections.filter(section=>section.id!=="rage"||hasRage)}),[hasRage]);
  const triggerHelp=module?.navigationHelp?.["/triggerbot"];
  const accessories=triggerHelp?{triggerbot:<NavigationHelp page="Triggerbot" content={triggerHelp}/>}:{ };
  if(params.get("section")?.split("/")[0]==="rage"&&!hasRage)return <Navigate to="/aim" replace/>;
  return <PageScaffold group={group} views={views} icons={icons} accessories={accessories} className="aim-page"/>;
}
