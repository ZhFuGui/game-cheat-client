import {nativeSnapshotSchema,sourceDataSchema} from "@mousebox/shared";
import type {SmokeContext} from "./context";
import {navigate,requireHidden,waitDom} from "./dom";
export async function verifySourceData({window,native,container}:Pick<SmokeContext,"window"|"native"|"container">){
  requireHidden(window);
  const snapshot=nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
  const context=snapshot.workspace?.active??undefined;
  if(!context)throw new Error("Source inspection needs an active game");
  const actual=sourceDataSchema.parse(await container.invoke("reader.inspect",{},context));
  if(!actual.streams.length||actual.streams.some(s=>s.available)||actual.payload!==null)
    throw new Error("Isolated source inspection did not report the disconnected streams");
  const original=native.request.bind(native);let reads=0;
  native.request=async(name,args,timeout)=>{
    if(name!=="reader.inspect")return original(name,args,timeout);
    ++reads;
    return {streams:[{id:"snapshot",title:"CS2 完整快照",available:true,format:"protobuf"}],selected:"snapshot",revision:"synthetic:1",
      payload:JSON.stringify({sequence:"18446744073709551615",health:0,enabled:false}),unchanged:false,error:null};
  };
  try{
    // Exercise the visible-page polling branch without showing or focusing a
    // real window. Requests below remain synthetic and read-only.
    await window.webContents.executeJavaScript("void Object.defineProperty(document,'hidden',{configurable:true,value:false})");
    await navigate(window,"/?section=dashboard",".data-dashboard-panel");
    await window.webContents.executeJavaScript("document.querySelector('[data-dashboard-view=source]').click()");
    await waitDom(window,"document.querySelector('.json-tree')?.textContent.includes('18446744073709551615')","Source data tree did not render");
    const data=await window.webContents.executeJavaScript("({zero:document.querySelector('.json-tree').textContent.includes('health:0'),falseValue:document.querySelector('.json-tree').textContent.includes('enabled:false'),rows:document.querySelectorAll('.json-tree-row').length})") as {zero:boolean;falseValue:boolean;rows:number};
    if(!data.zero||!data.falseValue||data.rows>40)throw new Error("Source data changed literal values or disabled virtualization");
    await window.webContents.executeJavaScript("[...document.querySelectorAll('.source-controls button')].find(b=>/暂停刷新|Pause updates/.test(b.textContent)).click()");
    const paused=reads;await window.webContents.executeJavaScript("new Promise(r=>setTimeout(r,280))");
    if(reads!==paused)throw new Error("Paused source viewer kept polling");
    await navigate(window,"/",".reader-page");
    const closed=reads;await window.webContents.executeJavaScript("new Promise(r=>setTimeout(r,160))");
    if(reads!==closed)throw new Error("Closed source viewer kept polling");
    return {sourceTree:true,exact64Bit:true,zeroAndFalse:true,pauseStopsPolling:true,closeStopsPolling:true,
      nativeWaitingStreams:actual.streams.length,syntheticDisplayOnly:true,syntheticVisibilityOnly:true,physicalOutput:false};
  }finally{native.request=original;await window.webContents.executeJavaScript("delete document.hidden");}
}
