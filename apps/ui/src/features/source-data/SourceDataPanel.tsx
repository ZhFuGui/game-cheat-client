import {HelpTooltip} from "~/components/shared/HelpTooltip";
import {useState} from "react";
import {useTranslation} from "react-i18next";
import {Button} from "~/components/ui/button";
import {SearchInput} from "~/components/ui/input";
import {Select} from "~/components/ui/select";
import {EmptyState} from "~/components/shared/Content";
import {JsonTree} from "~/components/shared/JsonTree";
import {invoke} from "~/lib/desktopApi";
import {useSourceData} from "./useSourceData";
export default function SourceDataPanel(){
  const {t}=useTranslation(),source=useSourceData();
  const [query,setQuery]=useState(""),[copied,setCopied]=useState<string|null>(null),[copyError,setCopyError]=useState("");
  const selected=source.reply?.streams.find(s=>s.id===source.reply?.selected);
  const keys=source.value!==null&&typeof source.value==="object"?Object.keys(source.value):[];
  const copy=async()=>{if(!source.text)return;try{await invoke("clipboard.write",{text:source.text});setCopied(source.reply?.revision??null);setCopyError("");}
    catch(error){setCopyError(error instanceof Error?error.message:String(error));}};
  return <section className="source-data-panel" aria-label={t("源数据")}>
    <div className="source-controls">
      <div className="source-stream"><Select label={t("订阅数据流")} value={source.stream||source.reply?.selected||""}
        options={(source.reply?.streams??[]).map(s=>({value:s.id,label:t(s.title)}))} onChange={v=>{source.setStream(String(v));setQuery("");setCopied(null);}}/><HelpTooltip label="源数据"><p>{t(selected?.format==="materialized"?"Reader 增量合并后的当前状态，分别查看 Living 与 Local；不是单条增量报文。":"实际收到的 Reader 完整快照；保留协议字段名，不加入页面计算值。")}</p><p>{t(selected?.format==="materialized"?"64位整数以字符串保留精度；字段值及缺失状态沿用 SDK 当前状态。":"64位整数以字符串保留精度；未发送字段不填默认值，二进制字段以十六进制显示。")}</p></HelpTooltip></div>
      <SearchInput aria-label={t("搜索字段或值")} placeholder={t("搜索字段或值")} value={query} onChange={e=>setQuery(e.target.value)}/>
      <div className="actions source-actions">
        <Button variant="outline" aria-pressed={source.paused} onClick={()=>{source.setPaused(!source.paused);setCopied(null);}}>{t(source.paused?"继续刷新":"暂停刷新")}</Button>
        <Button variant="outline" disabled={!source.text} onClick={()=>void copy()}>{t(copied!==null&&copied===source.reply?.revision?"已复制":"复制 JSON")}</Button>
      </div>
    </div>
    <div className="source-caption"><span data-paused={source.paused || undefined}>{t(source.paused?"已暂停，保留当前取样":"仅展示最新取样，每120毫秒刷新")}</span>
      <span title={source.reply?.revision??""}>{t("取样标识")}：{source.reply?.revision?.split(":").slice(-1)[0]??"—"}</span>
    </div>
    {(source.error||copyError)&&<p className="source-error" role="alert">{source.error||copyError}</p>}
    {!!keys.length&&<div className="source-keys"><span>{t("结构键")} {keys.length}</span>{keys.map(key=><span key={key}>{key}</span>)}</div>}
    {source.value===undefined?<EmptyState className="source-empty" icon="source" message={t(source.error?"源数据暂不可用":selected?.available?"正在整理源数据…":"等待当前订阅数据")}/>:
      <JsonTree value={source.value} query={query}/>}
  </section>;
}
