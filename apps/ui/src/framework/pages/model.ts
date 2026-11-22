import type {TabDefinition} from "~/framework/tabs/model";

/** One row of UI subpages. Content view modes belong inside the selected page. */
export interface PageSection extends TabDefinition {
  label:string;
  icon?:string;
  view?:string;
  context?:string;
  queryKeys?:readonly string[];
  panelClassName?:string;
  attributes?:Record<`data-${string}`,string>;
  children?:never;
  sections?:never;
}
export interface PageGroup {
  id:string;route:string;label:string;tabsLabel?:string;context?:string;
  sections:readonly PageSection[];defaultSection?:string;queryKey?:string;
}
export interface PageSelection {section:PageSection;context?:string}
export function pageQueryMemoryKey(group:string,section:string,key:string):string{
  return `${group}:${section}:${key}`;
}
export function definePageGroup<T extends PageGroup>(group:T):T{
  const validId=(id:string)=>/^[a-z0-9][a-z0-9-]*$/.test(id);
  if(!validId(group.id)||!group.sections.length||new Set(group.sections.map(section=>section.id)).size!==group.sections.length||
    group.defaultSection&&!group.sections.some(section=>section.id===group.defaultSection))throw new Error(`Invalid page group: ${group.id}`);
  for(const section of group.sections){
    if(!validId(section.id)||"children" in section||"sections" in section)throw new Error(`Nested page tabs are unsupported: ${section.id}`);
    if(section.placement!==undefined&&section.placement!=="start"&&section.placement!=="end")
      throw new Error(`Invalid tab placement: ${section.id}`);
  }
  return group;
}
export function resolvePageGroup(group:PageGroup,raw:string|null|undefined):PageSelection{
  // Old nested URLs still identify their primary section; the content owns view migration.
  const requested=raw?.split("/")[0];
  const section=group.sections.find(section=>section.id===requested)??
    group.sections.find(section=>section.id===group.defaultSection)??group.sections[0];
  return {section,context:section.context??group.context};
}
export function pageQuery(group:PageGroup,id:string):string{
  if(!group.sections.some(section=>section.id===id))throw new Error(`Unknown page section: ${id}`);
  return id===resolvePageGroup(group,"").section.id?"":id;
}
export function pageHref(group:PageGroup,raw:string):string{
  const section=pageQuery(group,resolvePageGroup(group,raw).section.id),query=new URLSearchParams();
  if(section)query.set(group.queryKey??"section",section);return group.route+(query.size?`?${query}`:"");
}
