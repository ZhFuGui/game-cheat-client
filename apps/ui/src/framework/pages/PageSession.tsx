import {createContext,useContext,useRef,type ReactNode} from "react";
const PageSession=createContext<Map<string,string>|null>(null);
/** Lives inside the workspace epoch boundary; stores navigation choices, never page contents. */
export function PageSessionProvider({children}:{children:ReactNode}){
  const memory=useRef(new Map<string,string>());
  return <PageSession.Provider value={memory.current}>{children}</PageSession.Provider>;
}
export function usePageSession(){
  const shared=useContext(PageSession),local=useRef(new Map<string,string>());
  return shared??local.current;
}
