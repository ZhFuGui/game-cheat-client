import {nativeSnapshotSchema} from "@mousebox/shared";
import type {SmokeContext} from "./context";
import {navigate,requireHidden,waitDom} from "./dom";
export async function verifyEspTiming({window,native,container}:Pick<SmokeContext,"window"|"native"|"container">){
  requireHidden(window);
  const snapshot=nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
  const context=snapshot.workspace?.active??undefined;
  if(!context)throw new Error("ESP timing verification requires an active game context");
  await navigate(window,"/visuals",".esp-page");
  try{
    container.publish("state",{...snapshot.state,espVisible:"SYNTHETIC-PERSON-COUNT",
      espTiming:{windowMs:2000,completionAvailable:true,processingMs:8.125,drawingMs:2.5,samples:12}},context);
    await waitDom(window,"document.querySelector('[data-esp-timing=processing]')?.textContent.includes('8.13') && document.querySelector('[data-esp-timing=drawing]')?.textContent.includes('2.50')","ESP paired timing values did not render");
    if(await window.webContents.executeJavaScript("document.querySelector('.esp-status').textContent.includes('SYNTHETIC-PERSON-COUNT')"))
      throw new Error("ESP toolbar retained the removed person counter");
    return {twoStageDisplay:true,windowMs:2000,countsRemoved:true,syntheticDisplayOnly:true,physicalOutput:false};
  }finally{container.publish("state",snapshot.state,context);}
}
