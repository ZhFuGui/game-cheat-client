import {Tooltip as Primitive} from "@base-ui/react/tooltip";
import {useEffect,useId,useState,type ReactNode} from "react";
import {useSurfaceStore} from "~/stores/surfaceStore";
export function Tooltip({label,content,children,className=""}:{label:string;content:ReactNode;children:ReactNode;className?:string}){
  const owner=useId();
  const [open,setOpen]=useState(false);
  useEffect(()=>()=>useSurfaceStore.getState().setOccluded(false,owner),[owner]);
  return <Primitive.Provider delay={250} closeDelay={100}><Primitive.Root onOpenChange={value=>{setOpen(value);useSurfaceStore.getState().setOccluded(value,owner);}}>
    <Primitive.Trigger aria-label={label} aria-describedby={open?owner:undefined} className={className}>{children}</Primitive.Trigger>
    <Primitive.Portal><Primitive.Positioner side="right" align="center" sideOffset={12} className="ui-tooltip-positioner">
      <Primitive.Popup id={owner} role="tooltip" className="ui-tooltip">{content}</Primitive.Popup>
    </Primitive.Positioner></Primitive.Portal>
  </Primitive.Root></Primitive.Provider>;
}
