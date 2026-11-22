import {isMovableTab, type TabDefinition} from "./model";

/** Saved IDs express preferences only; declarations own membership and fixed placement. */
export function reconcileTabOrder(tabs: readonly TabDefinition[], saved: readonly string[] = []): string[] {
  const movable = tabs.filter(isMovableTab), known = new Set(movable.map(tab => tab.id));
  const seen = new Set<string>(), middle: string[] = [];
  for (const id of [...saved, ...movable.map(tab => tab.id)]) {
    if (!known.has(id) || seen.has(id)) continue;
    seen.add(id); middle.push(id);
  }
  return [...tabs.filter(tab => tab.placement === "start").map(tab => tab.id), ...middle,
    ...tabs.filter(tab => tab.placement === "end").map(tab => tab.id)];
}

/** All reorder inputs share the same boundaries, including keyboard and persisted requests. */
export function moveTab(tabs: readonly TabDefinition[], id: string, index: number): string[] {
  const order = reconcileTabOrder(tabs), tab = tabs.find(item => item.id === id);
  if (!tab || !isMovableTab(tab) || !Number.isFinite(index)) return order;
  const first = tabs.filter(item => item.placement === "start").length;
  const last = order.length - tabs.filter(item => item.placement === "end").length - 1;
  const others = order.filter(value => value !== id);
  others.splice(Math.max(first, Math.min(last, Math.trunc(index))), 0, id);
  return others;
}
