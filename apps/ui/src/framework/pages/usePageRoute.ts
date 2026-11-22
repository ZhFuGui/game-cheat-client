import {useMemo} from "react";
import {useSearchParams} from "react-router";
import {pageQuery,pageQueryMemoryKey,resolvePageGroup,type PageGroup} from "./model";
import {usePageSession} from "./PageSession";
export function usePageRoute(group:PageGroup){
  const [params,setParams]=useSearchParams(),key=group.queryKey??"section";
  const memory=usePageSession();
  const raw=params.get(key);
  const selection=useMemo(()=>resolvePageGroup(group,raw),[group,raw]);
  const select=(id:string)=>{
    const section=pageQuery(group,id);
    setParams(previous=>{
      const next=new URLSearchParams(previous);
      for(const owned of selection.section.queryKeys??[]){
        const value=previous.get(owned);
        if(value!==null)memory.set(pageQueryMemoryKey(group.id,selection.section.id,owned),value);
      }
      for(const other of group.sections)if(other.id!==id)for(const owned of other.queryKeys??[])next.delete(owned);
      const destination=group.sections.find(item=>item.id===id)!;
      for(const owned of destination.queryKeys??[]){
        const remembered=memory.get(pageQueryMemoryKey(group.id,id,owned));
        if(!next.has(owned)&&remembered!==undefined)next.set(owned,remembered);
      }
      if(section)next.set(key,section);else next.delete(key);return next;
    });
  };
  return {selection,select};
}
