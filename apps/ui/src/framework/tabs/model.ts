/** Fixed regions use declaration order; only the middle region is user-sortable. */
export type TabPlacement = "start" | "end";

export interface TabDefinition {
  id: string;
  placement?: TabPlacement;
}

export function isMovableTab(tab: TabDefinition): boolean {
  return tab.placement === undefined;
}

export function canReorderTab(tabs: readonly TabDefinition[], id: string): boolean {
  const tab = tabs.find(item => item.id === id);
  return !!tab && isMovableTab(tab) && tabs.filter(isMovableTab).length > 1;
}
