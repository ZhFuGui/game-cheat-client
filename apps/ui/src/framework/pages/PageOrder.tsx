import { createContext, useContext, useState, type ReactNode } from "react";
import { reconcileTabOrder } from "~/framework/tabs/order";
import type {TabDefinition} from "~/framework/tabs/model";

type TabOrders = Readonly<Record<string, readonly string[]>>;
interface PageOrderPort {
  orders: TabOrders;
  onReorder: (scope: string, ids: string[]) => void;
  onDragChange?: (active: boolean, owner: string) => void;
}
const PageOrder = createContext<PageOrderPort | null>(null);
/** The shell supplies persistence; the page framework knows only stable group/tab IDs. */
export function PageOrderProvider({ children, ...port }: PageOrderPort & { children: ReactNode }) {
  return <PageOrder.Provider value={port}>{children}</PageOrder.Provider>;
}
export function usePageOrder(scope: string, tabs: readonly TabDefinition[]) {
  const port = useContext(PageOrder), [local, setLocal] = useState<string[]>([]);
  const current = reconcileTabOrder(tabs, port ? port.orders[scope] : local);
  return {
    order: current,
    reorder: (next: string[]) => {
      const order = reconcileTabOrder(tabs, next);
      if(order.every((id,index)=>id===current[index]))return;
      if (port) port.onReorder(scope, order);
      else setLocal(order);
    },
    onDragChange: port?.onDragChange,
  };
}
