import {lazy,Suspense,useEffect} from "react";
import {useSearchParams} from "react-router";
import {useTranslation} from "react-i18next";
import {AppIcon} from "~/components/icons";
import {usePageSession} from "~/framework/pages/PageSession";
import {pageQueryMemoryKey} from "~/framework/pages/model";
import {readerPages} from "~/navigation/pageGroups";
const Summary=lazy(()=>import("./GameDataView"));
const Source=lazy(()=>import("~/features/source-data/SourceDataPanel"));
const memoryKey=pageQueryMemoryKey(readerPages.id,"dashboard","view");

export default function DataDashboard(){
  const {t}=useTranslation(),[params,setParams]=useSearchParams(),memory=usePageSession();
  const requested=params.get("view")??params.get("section")?.split("/")[1];
  const mode=requested?(requested==="source"?"source":"summary"):(memory.get(memoryKey)==="source"?"source":"summary");
  useEffect(()=>{
    memory.set(memoryKey,mode);
    // A late lazy mount must not replace navigation to another page. Old URLs
    // are already readable; only an explicit view selection writes the route.
  },[memory,mode]);
  const select=(view:string)=>setParams(previous=>{const next=new URLSearchParams(previous);next.set("section","dashboard");next.set("view",view);return next;});
  return <>
    <div className="tabs" role="group" aria-label={t("看板内容")}>
      {(["summary","source"] as const).map(view=><button key={view} type="button" className={mode===view?"active":""}
        data-dashboard-view={view} aria-pressed={mode===view} onClick={()=>select(view)}>
        <AppIcon name={view==="summary"?"summary":"source"} size={14}/>{t(view==="summary"?"摘要":"源数据")}
      </button>)}
    </div>
    <Suspense fallback={null}>{mode==="source"?<Source/>:<Summary/>}</Suspense>
  </>;
}
