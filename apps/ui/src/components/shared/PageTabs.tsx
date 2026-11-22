import {useRef,type ReactNode} from "react";
import {useTranslation} from "react-i18next";
import {canReorderTab, type TabDefinition} from "~/framework/tabs/model";
import {moveTab,reconcileTabOrder} from "~/framework/tabs/order";
import {useTabReorder} from "./page-tabs/useTabReorder";

export interface PageTab extends TabDefinition {
  label:string;
  icon?:ReactNode;
  attributes?:Record<`data-${string}`,string>;
  accessory?:ReactNode;
}

/** Reorderable document tabs. Routing and panel lifetime belong to the page. */
export function PageTabs({scope,label,items:definitions,selected,onSelect,onReorder,onDragChange}:{
  scope:string;label:string;items:readonly PageTab[];selected:string;onSelect:(id:string)=>void;
  onReorder?:(ids:string[])=>void;onDragChange?:(active:boolean,owner:string)=>void;
}){
  const {t}=useTranslation(),list=useRef<HTMLDivElement>(null);
  const items=reconcileTabOrder(definitions).map(id=>definitions.find(item=>item.id===id)!);
  const ids=items.map(item=>item.id);
  const drag=useTabReorder({list,items,onReorder,onDragChange});
  return <div className="page-tabs" role="tablist" aria-label={t(label)} ref={list}>
    <div className="page-tabs-track" role="presentation">
      {items.map((item,index)=><div key={item.id} className="page-tab-item" data-has-accessory={!!item.accessory||undefined}>
        <button type="button" className="page-tab" {...item.attributes}
        id={`${scope}-tab-${item.id}`} role="tab" aria-selected={selected===item.id}
        aria-controls={`${scope}-panel-${item.id}`} tabIndex={selected===item.id?0:-1}
        data-tab-id={item.id} data-tab-placement={item.placement} data-reorderable={!!onReorder&&canReorderTab(items,item.id)||undefined}
        onPointerDown={event=>{event.currentTarget.dataset.pointerFocus="true";drag.start(event,item.id);}}
        onBlur={event=>{delete event.currentTarget.dataset.pointerFocus;}} onDragStart={event=>event.preventDefault()}
        aria-keyshortcuts={onReorder&&canReorderTab(items,item.id)?"Alt+Shift+ArrowLeft Alt+Shift+ArrowRight":undefined}
        data-before-active={items[index+1]?.id===selected||undefined}
        onClick={event=>{if(!drag.consumeClick(event))onSelect(item.id);}} onKeyDown={event=>{
          delete event.currentTarget.dataset.pointerFocus;
          if(onReorder&&event.altKey&&event.shiftKey&&(event.key==="ArrowRight"||event.key==="ArrowLeft")){
            event.preventDefault();
            if(!canReorderTab(items,item.id))return;
            const target=Math.max(0,Math.min(items.length-1,index+(event.key==="ArrowRight"?1:-1)));
            const next=moveTab(items,item.id,target);
            if(next.some((id,position)=>id!==ids[position]))onReorder(next);
            return;
          }
          let next=index;
          if(event.key==="ArrowRight")next=(index+1)%items.length;
          else if(event.key==="ArrowLeft")next=(index+items.length-1)%items.length;
          else if(event.key==="Home")next=0;
          else if(event.key==="End")next=items.length-1;
          else return;
          event.preventDefault();onSelect(items[next].id);
          list.current?.querySelectorAll<HTMLButtonElement>('[role="tab"]')[next]?.focus();
        }}>
        {item.icon&&<span className="page-tab-icon" aria-hidden="true">{item.icon}</span>}
        <span className="page-tab-label" title={t(item.label)}>{t(item.label)}</span>
        </button>
        {item.accessory&&<div className="page-tab-accessory">{item.accessory}</div>}
      </div>)}
    </div>
  </div>;
}
