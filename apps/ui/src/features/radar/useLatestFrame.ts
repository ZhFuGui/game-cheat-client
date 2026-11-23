import {useEffect,useState} from "react";

/** Bounded pull loop. Hiding, leaving or replacing a scope invalidates pending work. */
export function useLatestFrame<T>(read: ((after?:string)=>Promise<{frame:T|null;unchanged:boolean}>) | null,
  getRevision: (frame:T)=>string) {
  const [frame,setFrame]=useState<T|null>(null),[failed,setFailed]=useState(false);
  useEffect(()=>{
    setFrame(null);setFailed(false);
    if(!read)return;
    let stopped=false,reading=false,generation=0;
    let after:string|undefined,timer:ReturnType<typeof setTimeout>|undefined;
    const poll=async()=>{
      if(stopped||reading||document.hidden)return;
      reading=true;const started=performance.now(),request=generation;
      let delay=1000/30;
      try{
        const result=await read(after);
        if(stopped||document.hidden||generation!==request)return;
        if(!result.unchanged){after=result.frame?getRevision(result.frame):undefined;setFrame(result.frame);}
        setFailed(false);if(!after)delay=180;
      }catch{
        if(stopped||document.hidden||generation!==request)return;
        after=undefined;setFrame(null);setFailed(true);delay=600;
      }finally{
        reading=false;
        if(!stopped&&!document.hidden)timer=setTimeout(()=>void poll(),Math.max(0,delay-(performance.now()-started)));
      }
    };
    const visibility=()=>{clearTimeout(timer);++generation;after=undefined;setFrame(null);setFailed(false);if(!document.hidden)void poll();};
    document.addEventListener("visibilitychange",visibility);void poll();
    return()=>{stopped=true;clearTimeout(timer);document.removeEventListener("visibilitychange",visibility);};
  },[read,getRevision]);
  return{frame:read?frame:null,failed:!!read&&failed};
}
