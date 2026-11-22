import {writeFile} from "node:fs/promises";
import {dirname,join} from "node:path";
import {nativeSnapshotSchema,type WorkspaceContext} from "@mousebox/shared";
import type {SmokeContext} from "./context";
import {navigate,requireHidden,waitDom} from "./dom";

/** Settings-only checks in an isolated hidden window; no source/device output. */
export async function verifyAimMovement({window,native,args,report}:Pick<SmokeContext,"window"|"native"|"args"|"report">,context:WorkspaceContext) {
  requireHidden(window);
  await navigate(window,"/aim?section=general",".aim-general-page");
  const read=async()=>nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
  const original=await read();
  if(original.state.readerConnected||original.state.boxReady||original.state.aimArmed)
    throw new Error("Movement verification requires disconnected sources and devices");
  const panel="document.querySelector('.mouse-movement-algorithms')";
  const field=(label:string)=>"document.getElementById([..."+panel+".querySelectorAll('label')].find(e=>e.textContent==="+JSON.stringify(label)+")?.htmlFor)";
  const set=(key:string,value:unknown)=>window.webContents.executeJavaScript(
    "window.mousebox.invoke('settings.set',"+JSON.stringify({key,value})+","+JSON.stringify(context)+")");
  const choose=async(label:string,text:string,count:number)=>{
    const selector=field(label);
    await waitDom(window,selector+"?.disabled===false","Selector did not enable: "+label);
    await window.webContents.executeJavaScript(selector+".click()");
    const popup="document.getElementById("+selector+".getAttribute('aria-controls'))";
    await waitDom(window,popup+"?.querySelectorAll('[role=option]').length==="+count,"Incorrect choices for "+label);
    await window.webContents.executeJavaScript("(()=>{const e=[..."+popup+".querySelectorAll('[role=option]')].find(e=>e.textContent==="+JSON.stringify(text)+");e.dispatchEvent(new PointerEvent('pointerdown',{bubbles:true,pointerType:'touch'}));e.click()})()");
    await waitDom(window,selector+"?.textContent==="+JSON.stringify(text)+"&&"+selector+"?.disabled===false","Choice did not persist: "+label);
  };
  const keys=["mouseMovementAlgorithm","mouseTrajectory","mouseBezierControl1X","mouseBezierControl1Y","mouseBezierControl2X","mouseBezierControl2Y"] as const;
  const previews:string[]=[];
  try {
    for(const [id,label] of [["smooth","平滑加减速"],["recoil-tracking","后坐优先响应"],["direct","直接响应"]] as const) {
      await choose("速度变化控制",label,3);
      const actual=await read();
      if(actual.values.mouseMovementAlgorithm!==id||actual.values.aimVersion!=="v1"||actual.values.speed!==original.values.speed)
        throw new Error("Speed control was not independent: "+id);
    }
    await choose("移动轨迹","贝塞尔曲线",2);
    await waitDom(window,"!!document.querySelector('.mouse-bezier-preview')&&document.querySelectorAll('.mouse-bezier-settings input').length===4","Bezier controls did not mount");
    for(const [key,value] of [["mouseBezierControl1X",15],["mouseBezierControl1Y",35],["mouseBezierControl2X",85],["mouseBezierControl2Y",-20]] as const)
      await set(key,value);
    await waitDom(window,"[...document.querySelectorAll('.mouse-bezier-settings input')].map(e=>e.value).join(',')==='15,35,85,-20'","Native control points did not reach the renderer");
    const actual=await read();
    if(actual.values.mouseTrajectory!=="bezier"||actual.values.mouseMovementAlgorithm!=="direct"||actual.values.aimVersion!=="v1")
      throw new Error("Curve configuration changed recoil or speed selection");
    if(args.includes("--movement-preview")) {
      requireHidden(window);
      await window.webContents.executeJavaScript("document.querySelector('.mouse-movement-algorithms').scrollIntoView({block:'start',behavior:'instant'})");
      await window.webContents.capturePage(undefined,{stayHidden:true,stayAwake:true});
      await window.webContents.executeJavaScript("new Promise(resolve=>setTimeout(resolve,200))");
      const image=await window.webContents.capturePage(undefined,{stayHidden:true,stayAwake:true});
      if(image.isEmpty())throw new Error("Owned hidden movement preview was empty");
      const file=join(dirname(report),"aim-movement.png");await writeFile(file,image.toPNG());previews.push(file);
    }
    await choose("移动轨迹","直线",2);
    await waitDom(window,"!document.querySelector('.mouse-bezier-settings')","Linear mode retained curve editors");
    if((await read()).values.mouseBezierControl2Y!==-20)throw new Error("Linear mode erased saved handles");
  } finally {
    for(const key of keys)await set(key,original.values[key]);
  }
  const restored=await read();
  if(JSON.stringify(restored.values)!==JSON.stringify(original.values))throw new Error("Movement checks did not restore isolated settings");
  requireHidden(window);
  return {choices:3,trajectories:2,independent:true,bezierControls:true,restored:true,previews,physicalOutput:false};
}
