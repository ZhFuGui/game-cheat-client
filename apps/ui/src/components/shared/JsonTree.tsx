import {useEffect,useMemo,useRef,useState} from "react";
import {useTranslation} from "react-i18next";
import {AppIcon} from "~/components/icons";
import {jsonPreview,jsonRows} from "./jsonTreeModel";
const HEIGHT=24;
export function JsonTree({value,query=""}:{value:unknown;query?:string}){
  const {t}=useTranslation();
  const [expanded,setExpanded]=useState(()=>new Set([""]));
  const [scroll,setScroll]=useState(0),[height,setHeight]=useState(420);
  const viewport=useRef<HTMLDivElement>(null);
  const rows=useMemo(()=>jsonRows(value,expanded,query),[value,expanded,query]);
  useEffect(()=>{const node=viewport.current;if(!node||typeof ResizeObserver==="undefined")return;
    const observer=new ResizeObserver(()=>setHeight(node.clientHeight||420));observer.observe(node);
    return()=>observer.disconnect();},[]);
  useEffect(()=>{if(viewport.current)viewport.current.scrollTop=0;setScroll(0);},[query]);
  const start=Math.max(0,Math.min(Math.floor(scroll/HEIGHT)-5,Math.max(0,rows.length-1)));
  const visible=rows.slice(start,start+Math.ceil(height/HEIGHT)+10);
  const toggle=(path:string)=>setExpanded(previous=>{const next=new Set(previous);if(!next.delete(path))next.add(path);return next;});
  return <div className="json-tree" ref={viewport} onScroll={e=>setScroll(e.currentTarget.scrollTop)}
    role="region" aria-label={t("源数据字段树")} tabIndex={0}>
    {!rows.length?<p className="source-empty">{t("没有匹配字段")}</p>:<div className="json-tree-scroll" style={{height:rows.length*HEIGHT}}>
      <div className="json-tree-window" style={{top:start*HEIGHT}}>{visible.map(row=>
        <div className="json-tree-row" key={row.path} style={{paddingLeft:8+Math.min(row.depth,24)*16,height:HEIGHT}} title={row.path||"$"}>
          {row.branch?<button type="button" aria-expanded={expanded.has(row.path)} aria-label={`${t("展开或收起")} ${row.path||"$"}`}
            onClick={()=>toggle(row.path)}><AppIcon name={expanded.has(row.path)?"chevronDown":"chevronRight"} size={12}/></button>:<span className="json-tree-spacer"/>}
          <span className="json-tree-key">{row.key}</span><span className="json-tree-colon">:</span>
          <span className={`json-tree-value json-${row.value===null?"null":typeof row.value}`}>{jsonPreview(row.value)}</span>
        </div>)}</div>
    </div>}
  </div>;
}
