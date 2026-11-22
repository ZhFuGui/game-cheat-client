import {preferencesSchema,type UiPreferences} from "@mousebox/shared";
import type {SmokeContext} from "./context";
import {requireHidden,waitDom} from "./dom";
export async function verifyEspOutput({window,native,container}:Pick<SmokeContext,"window"|"native"|"container">,original:UiPreferences){
  requireHidden(window);
  const set=(value:object)=>container.invoke("preferences.set",value);
  try{
    await window.webContents.executeJavaScript("document.querySelector('[data-esp-output-toggle]').click()");
    await waitDom(window,"document.querySelectorAll('.esp-output-settings [role=combobox]').length===3","ESP output controls did not load");
    const saved=await set({espPlacement:"overlay",espCapture:"exclude",espRenderer:"software"});
    await waitDom(window,"!!document.querySelector('.esp-overlay-placeholder')","Overlay placeholder did not appear");
    const info=await native.request("render.inspect") as {surface:{placement:string;requested:boolean;overlay?:{visible:boolean;captureApplied:boolean}};initialized:boolean};
    if(info.surface.placement!=="overlay"||info.surface.requested||info.surface.overlay?.visible||info.initialized)
      throw new Error("Hidden verification attempted to display or render a game overlay");
    const prefs=preferencesSchema.parse(saved);
    if(prefs.espPlacement!=="overlay"||prefs.espCapture!=="exclude"||prefs.espRenderer!=="software")
      throw new Error("ESP output preferences did not roundtrip through the actual bridge");
    return {controls:true,preferences:true,capturePolicyIndependent:true,systemExclusionApplied:info.surface.overlay?.captureApplied??false,
      softwareSelection:true,overlayShown:false,physicalOutput:false};
  }finally{
    await set({espPlacement:original.espPlacement??"embedded",espCapture:original.espCapture??"include",espRenderer:original.espRenderer??"default"});
    await window.webContents.executeJavaScript("document.querySelector('[data-esp-output-toggle][aria-expanded=true]')?.click()");
    await waitDom(window,"!document.querySelector('.esp-output-settings') && !!document.querySelector('.esp-overlay-placeholder')==="+(original.espPlacement==="overlay"),"ESP output verification did not restore the original view");
  }
}
