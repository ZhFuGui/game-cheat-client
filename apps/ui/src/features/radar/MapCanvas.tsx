import {useEffect, useRef, useState, type KeyboardEvent, type PointerEvent} from "react";
import {useTranslation} from "react-i18next";

export interface MapMarker {
  key: string;
  x: number;
  y: number;
  color: string;
  label?: string;
  local?: boolean;
}
const clamp = (value: number, low: number, high: number) => Math.max(low, Math.min(high,value));

/** Image and normalized markers only. Game coordinates and identity stay with the caller. */
export function MapCanvas({imageUrl, markers, centerOn}: {
  imageUrl?: string;
  markers: readonly MapMarker[];
  centerOn?: readonly [number,number];
}) {
  const {t} = useTranslation();
  const canvas = useRef<HTMLCanvasElement>(null);
  const drag = useRef<{id:number; x:number;y:number} | null>(null);
  const [image,setImage] = useState<HTMLImageElement | null>(null);
  const [failed,setFailed] = useState(false);
  const [size,setSize] = useState({width:0,height:0});
  const [view,setView] = useState({x:0.5,y:0.5,zoom:1});
  const extent = Math.min(size.width,size.height) * view.zoom;
  const origin = centerOn ?? [view.x,view.y];
  const pan = (dx:number,dy:number) => setView(value=>({...value,
    x:clamp(value.x-dx/(extent||1),-0.5,1.5),y:clamp(value.y-dy/(extent||1),-0.5,1.5)}));

  useEffect(()=>{
    if(!imageUrl){setImage(null);setFailed(false);return;}
    const texture=new Image();let active=true;
    setImage(null);setFailed(false);
    texture.onload=()=>{if(active)setImage(texture);};
    texture.onerror=()=>{if(active)setFailed(true);};
    texture.src=imageUrl;
    return()=>{active=false;texture.onload=texture.onerror=null;};
  },[imageUrl]);
  useEffect(()=>{
    const element=canvas.current;if(!element)return;
    const measure=()=>setSize({width:element.clientWidth,height:element.clientHeight});
    const observer=new ResizeObserver(measure);observer.observe(element);measure();
    return()=>observer.disconnect();
  },[]);
  useEffect(()=>{
    const element=canvas.current;if(!element)return;
    const wheel=(event:WheelEvent)=>{event.preventDefault();setView(value=>({...value,zoom:clamp(value.zoom*(event.deltaY<0?1.12:1/1.12),0.5,6)}));};
    element.addEventListener("wheel",wheel,{passive:false});
    return()=>element.removeEventListener("wheel",wheel);
  },[]);
  useEffect(()=>{
    const element=canvas.current;if(!element||!size.width||!size.height)return;
    const drawing=window.requestAnimationFrame(()=>{
      const ratio=window.devicePixelRatio||1;
      element.width=Math.round(size.width*ratio);element.height=Math.round(size.height*ratio);
      const context=element.getContext("2d");if(!context)return;
      context.setTransform(ratio,0,0,ratio,0,0);
      context.clearRect(0,0,size.width,size.height);
      const left=size.width/2-origin[0]*extent,top=size.height/2-origin[1]*extent;
      if(image)context.drawImage(image,left,top,extent,extent);
      context.font=`12px ${getComputedStyle(element).fontFamily}`;context.textAlign="left";context.textBaseline="middle";
      for(const marker of markers){
        const x=left+marker.x*extent,y=top+marker.y*extent;
        if(!Number.isFinite(x)||!Number.isFinite(y)||x<0||y<0||x>size.width||y>size.height)continue;
        context.beginPath();context.arc(x,y,marker.local?6:4,0,Math.PI*2);
        context.fillStyle=marker.color;context.fill();context.lineWidth=1.5;context.strokeStyle="#0b1520";context.stroke();
        if(marker.local){context.beginPath();context.arc(x,y,9,0,Math.PI*2);context.strokeStyle=marker.color;context.stroke();}
        if(marker.label){context.lineWidth=3;context.strokeStyle="#0b1520";context.strokeText(marker.label,x+9,y);context.fillStyle="#f3f5f8";context.fillText(marker.label,x+9,y);}
      }
    });
    return()=>window.cancelAnimationFrame(drawing);
  },[extent,image,markers,origin,size]);

  const begin=(event:PointerEvent<HTMLCanvasElement>)=>{
    if(event.button!==0||centerOn)return;
    event.currentTarget.setPointerCapture(event.pointerId);drag.current={id:event.pointerId,x:event.clientX,y:event.clientY};
  };
  const move=(event:PointerEvent<HTMLCanvasElement>)=>{
    const previous=drag.current;if(previous?.id!==event.pointerId)return;
    pan(event.clientX-previous.x,event.clientY-previous.y);previous.x=event.clientX;previous.y=event.clientY;
  };
  const end=(event:PointerEvent<HTMLCanvasElement>)=>{
    if(drag.current?.id!==event.pointerId)return;
    drag.current=null;if(event.currentTarget.hasPointerCapture(event.pointerId))event.currentTarget.releasePointerCapture(event.pointerId);
  };
  const key=(event:KeyboardEvent<HTMLCanvasElement>)=>{
    if(event.key==="+"||event.key==="="){event.preventDefault();setView(value=>({...value,zoom:clamp(value.zoom*1.12,0.5,6)}));}
    else if(event.key==="-"){event.preventDefault();setView(value=>({...value,zoom:clamp(value.zoom/1.12,0.5,6)}));}
    else if(event.key==="Home"){event.preventDefault();setView({x:0.5,y:0.5,zoom:1});}
    else if(!centerOn&&["ArrowLeft","ArrowRight","ArrowUp","ArrowDown"].includes(event.key)){
      event.preventDefault();pan(event.key==="ArrowLeft"?24:event.key==="ArrowRight"?-24:0,event.key==="ArrowUp"?24:event.key==="ArrowDown"?-24:0);
    }
  };
  return <>
    <canvas ref={canvas} className="radar-canvas" data-markers={markers.length} data-map-ready={!imageUrl||!!image} tabIndex={0} role="img" aria-label={t("平面雷达地图")}
      onPointerDown={begin} onPointerMove={move} onPointerUp={end} onPointerCancel={end} onLostPointerCapture={()=>{drag.current=null;}} onKeyDown={key}/>
    {imageUrl&&!image&&<div className="radar-map-status" role="status">{t(failed?"雷达底图加载失败":"正在加载雷达底图…")}</div>}
  </>;
}
