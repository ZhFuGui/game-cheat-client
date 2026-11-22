export interface JsonRow {path:string;key:string;value:unknown;depth:number;branch:boolean}
const object=(v:unknown):v is Record<string,unknown>=>v!==null&&typeof v==="object";
const pointer=(s:string)=>s.replaceAll("~","~0").replaceAll("/","~1");
export function jsonRows(value:unknown,expanded:ReadonlySet<string>,query=""):JsonRow[]{
  const rows:JsonRow[]=[],needle=query.trim().toLocaleLowerCase();
  let visited=0;
  const walk=(v:unknown,path:string,key:string,depth:number)=>{
    if(++visited>400000||depth>64)return;
    const branch=object(v),row={path,key,value:v,depth,branch};
    if(!needle||key.toLocaleLowerCase().includes(needle)||path.toLocaleLowerCase().includes(needle)||
      !branch&&String(v).toLocaleLowerCase().includes(needle))rows.push(row);
    if(branch&&(needle||expanded.has(path)))
      for(const [k,child]of Object.entries(v))walk(child,`${path}/${pointer(k)}`,k,depth+1);
  };
  walk(value,"","$",0);
  return rows;
}
export function jsonPreview(value:unknown):string{
  if(Array.isArray(value))return `[${value.length}]`;
  if(value!==null&&typeof value==="object")return `{${Object.keys(value).length}}`;
  return JSON.stringify(value)??"null";
}
