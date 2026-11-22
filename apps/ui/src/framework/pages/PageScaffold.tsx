import {Suspense,type ComponentType,type ReactNode} from "react";
import {PageTabs} from "~/components/shared/PageTabs";
import type {PageGroup} from "./model";
import {usePageRoute} from "./usePageRoute";
import {usePageOrder} from "./PageOrder";
export type PageViews=Readonly<Record<string,ComponentType>>;
type PageIcons=Readonly<Record<string,ReactNode>>;
/** One tab row per page. Only the selected content is mounted. */
export function PageScaffold({group,views,icons={},accessories={},className=""}:{
  group:PageGroup;views:PageViews;icons?:PageIcons;accessories?:Readonly<Record<string,ReactNode>>;className?:string;
}){
  const {selection:{section},select}=usePageRoute(group);
  const {order,reorder,onDragChange}=usePageOrder(group.id,group.sections);
  const View=section.view?views[section.view]:undefined;
  if(section.view&&!View)throw new Error(`Unregistered page view: ${section.view}`);
  return <div className={`page browser-page page-scaffold ${className}`} data-page-group={group.id}>
    <PageTabs scope={group.id} label={group.tabsLabel??group.label}
      items={order.map(id=>{const section=group.sections.find(item=>item.id===id)!;return {id:section.id,label:section.label,placement:section.placement,icon:section.icon?icons[section.icon]:undefined,attributes:section.attributes,accessory:accessories[section.id]};})}
      selected={section.id} onSelect={select} onReorder={reorder} onDragChange={onDragChange}/>
    <div role="tabpanel" id={`${group.id}-panel-${section.id}`} aria-labelledby={`${group.id}-tab-${section.id}`}
      className={`browser-page-content ${section.panelClassName??""}`}>
      <Suspense fallback={null}>{View&&<View key={section.id}/>}</Suspense>
    </div>
  </div>;
}
