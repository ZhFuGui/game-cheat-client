import {useEffect,useRef,useState} from "react";
import {sameWorkspaceContext,type SourceData} from "@mousebox/shared";
import {invoke} from "~/lib/desktopApi";
import {useRuntimeStore} from "~/runtime/state";
export const SOURCE_SAMPLE_INTERVAL=120;
interface View {reply?:SourceData;value?:unknown;text?:string;error?:string}
export function useSourceData(){
  const context=useRuntimeStore(s=>s.workspace?.active),connected=useRuntimeStore(s=>s.connected);
  const key=context?`${context.workspaceId}:${context.epoch}`:"";
  const [stream,setStream]=useState(""),[paused,setPaused]=useState(false),[view,setView]=useState<View>({});
  const after=useRef("");
  useEffect(()=>{after.current="";setView({});},[key,stream]);
  useEffect(()=>{
    if(!connected){after.current="";setView({});return;}
    if(!context||paused)return;
    let stopped=false,timer:ReturnType<typeof setTimeout>|undefined,running=false;
    const poll=async()=>{
      if(stopped||running)return;
      if(document.hidden){timer=setTimeout(()=>void poll(),SOURCE_SAMPLE_INTERVAL);return;}
      running=true;let next=SOURCE_SAMPLE_INTERVAL;
      try{
        const reply=await invoke("reader.inspect",{stream,after:after.current},context);
        if(stopped||!sameWorkspaceContext(context,useRuntimeStore.getState().workspace?.active))return;
        const value=reply.payload===null?undefined:JSON.parse(reply.payload) as unknown;
        after.current=reply.revision??"";
        setView(previous=>({reply,value:reply.unchanged?previous.value:value,
          text:reply.unchanged?previous.text:reply.payload??undefined,error:reply.error??undefined}));
      }catch(error){if(!stopped){setView(previous=>({...previous,error:error instanceof Error?error.message:String(error)}));next=1000;}}
      finally{running=false;if(!stopped)timer=setTimeout(()=>void poll(),next);}
    };
    const visible=()=>{if(!document.hidden&&!running){clearTimeout(timer);void poll();}};
    document.addEventListener("visibilitychange",visible);void poll();
    return()=>{stopped=true;clearTimeout(timer);document.removeEventListener("visibilitychange",visible);};
  },[context,key,connected,stream,paused]);
  return {...view,stream,paused,setPaused,setStream:(value:string)=>{setStream(value);setPaused(false);}};
}
