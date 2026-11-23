import {useMemo} from "react";
import {sameWorkspaceContext,type ValorantRadarFrame} from "@mousebox/shared";
import {invoke} from "~/lib/desktopApi";
import {useRuntimeStore} from "~/runtime/state";
import {useLatestFrame} from "~/features/radar/useLatestFrame";

const revision=(frame:ValorantRadarFrame)=>frame.revision;
export function useRadarFrame(){
  const context=useRuntimeStore(state=>state.workspace?.active);
  const connected=useRuntimeStore(state=>state.connected);
  const refreshing=useRuntimeStore(state=>state.workspaceRefreshing);
  const read=useMemo(()=>connected&&!refreshing&&context?.gameId==="valorant"?async(after?:string)=>{
    const result=await invoke("game.valorant.radar.read",after?{after}:{},context);
    if(!sameWorkspaceContext(context,useRuntimeStore.getState().workspace?.active))throw Error("Expired radar scope");
    return result;
  }:null,[context,connected,refreshing]);
  return useLatestFrame(read,revision);
}
