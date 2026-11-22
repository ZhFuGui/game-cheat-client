import {writeFile} from "node:fs/promises";
import {dirname,join} from "node:path";
import {bootstrapSchema} from "@mousebox/shared";
import {UiPreferenceStore} from "../infrastructure/storage/UiPreferences";
import type {SmokeContext} from "./context";
import {navigate,requireHidden,waitDom} from "./dom";

/** Fixed placement and persistence in the packaged renderer, using DOM events only. */
export async function verifyFixedTabs({window,container,args,report}:Pick<SmokeContext,"window"|"container"|"args"|"report">) {
  requireHidden(window);
  const preferences=()=>container.invoke("bootstrap",{}).then(value=>bootstrapSchema.parse(value).preferences);
  const original=(await preferences()).pageTabOrder?.aim??[];
  const incoming=["rage","general","triggerbot","aimbot"], restored=["general","rage","triggerbot","aimbot"];
  const readOrder="[...document.querySelectorAll('.aim-page [role=tab]')].map(tab=>tab.dataset.tabId)";
  const waitOrder=(ids:string[])=>waitDom(window,`JSON.stringify(${readOrder})===${JSON.stringify(JSON.stringify(ids))}`,"Fixed tab order did not settle");
  try {
    await navigate(window,"/aim",".aimbot-page");
    await container.invoke("preferences.set",{pageTabOrder:{aim:incoming}});
    await waitOrder(restored);
    const fixed=await window.webContents.executeJavaScript(`(()=>{
      const page=document.querySelector('.aim-page'),tab=page.querySelector('[data-tab-id=general]');
      const panel=page.querySelector('[role=tabpanel]');panel.dataset.fixedTabProbe='retained';
      const selected=page.querySelector('[role=tab][aria-selected=true]').id;
      tab.focus({preventScroll:true});
      const rect=tab.getBoundingClientRect(),x=rect.left+rect.width/2,y=rect.top+rect.height/2;
      tab.dispatchEvent(new PointerEvent('pointerdown',{bubbles:true,cancelable:true,pointerId:41,button:0,buttons:1,clientX:x,clientY:y}));
      window.dispatchEvent(new PointerEvent('pointermove',{bubbles:true,cancelable:true,pointerId:41,buttons:1,clientX:x+650,clientY:y}));
      const noDrag=!page.querySelector('[data-reordering]');
      window.dispatchEvent(new PointerEvent('pointerup',{bubbles:true,pointerId:41,button:0,clientX:x+650,clientY:y}));
      tab.dispatchEvent(new KeyboardEvent('keydown',{bubbles:true,cancelable:true,key:'ArrowRight',altKey:true,shiftKey:true}));
      return {selected,noDrag,noReorderAttribute:!tab.hasAttribute('data-reorderable'),noShortcut:!tab.hasAttribute('aria-keyshortcuts'),
        noOutline:getComputedStyle(tab).outlineStyle==='none',noTextSelection:getComputedStyle(tab).userSelect==='none',placement:tab.dataset.tabPlacement};
    })()`) as {selected:string;noDrag:boolean;noReorderAttribute:boolean;noShortcut:boolean;noOutline:boolean;noTextSelection:boolean;placement:string};
    if(!fixed.noDrag||!fixed.noReorderAttribute||!fixed.noShortcut||!fixed.noOutline||!fixed.noTextSelection||fixed.placement!=="start")
      throw new Error("Fixed tab accepts reorder or text selection: "+JSON.stringify(fixed));
    await waitOrder(restored);
    if(JSON.stringify((await preferences()).pageTabOrder?.aim)!==JSON.stringify(incoming))
      throw new Error("Reading fixed placement or a blocked gesture rewrote saved preferences");
    const preview=await window.webContents.executeJavaScript(`(()=>{
      const page=document.querySelector('.aim-page'),tab=page.querySelector('[data-tab-id=aimbot]');
      const source=tab.getBoundingClientRect(),fixed=page.querySelector('[data-tab-id=general]').getBoundingClientRect();
      const x=source.left+source.width/2,y=source.top+source.height/2,end=fixed.left+fixed.width/2;
      tab.dispatchEvent(new PointerEvent('pointerdown',{bubbles:true,cancelable:true,pointerId:42,button:0,buttons:1,clientX:x,clientY:y}));
      window.dispatchEvent(new PointerEvent('pointermove',{bubbles:true,cancelable:true,pointerId:42,buttons:1,clientX:end,clientY:y}));
      const preview=page.querySelector('.page-tabs').dataset.reordering==='true';
      window.dispatchEvent(new PointerEvent('pointerup',{bubbles:true,pointerId:42,button:0,clientX:end,clientY:y}));
      tab.dispatchEvent(new MouseEvent('click',{bubbles:true,cancelable:true,detail:1}));
      return preview;
    })()`) as boolean;
    const expected=["general","aimbot","rage","triggerbot"];
    await waitOrder(expected);
    const stable=await window.webContents.executeJavaScript(`(()=>{
      const page=document.querySelector('.aim-page');return page.querySelector('[role=tab][aria-selected=true]').id===${JSON.stringify(fixed.selected)} &&
        page.querySelector('[role=tabpanel]').dataset.fixedTabProbe==='retained'&&!page.querySelector('[data-reordering]');
    })()`) as boolean;
    const directory=args.find(arg=>arg.startsWith("--data-dir="))?.slice("--data-dir=".length);
    if(!directory)throw new Error("Fixed tab verification requires isolated preferences");
    const saved=(await preferences()).pageTabOrder?.aim;
    const reopened=new UiPreferenceStore(join(directory,"web-ui.json")).get().pageTabOrder?.aim;
    if(!preview||!stable||JSON.stringify(saved)!==JSON.stringify(expected)||JSON.stringify(reopened)!==JSON.stringify(expected))
      throw new Error("Movable tabs crossed the fixed region or lost their selected content");
    await window.webContents.executeJavaScript("document.querySelector('.aim-page [data-tab-id=general]').click()");
    await waitDom(window,"document.querySelector('.aim-page [role=tabpanel]')?.id==='aim-panel-general' && !!document.querySelector('.aim-general-page .mouse-movement-algorithms') && document.querySelector('.page-context-bar')?.dataset.contextPage==='aim-general'","General tab did not display its independent shared modules");
    const previews:string[]=[];
    if(args.includes("--preview-aimbot")) {
      await window.webContents.capturePage(undefined,{stayHidden:true,stayAwake:true});
      await window.webContents.executeJavaScript("new Promise(resolve=>setTimeout(resolve,100))");
      const image=await window.webContents.capturePage(undefined,{stayHidden:true,stayAwake:true});
      if(image.isEmpty())throw new Error("Hidden general tab preview was empty");
      const file=join(dirname(report),"aim-general.png");await writeFile(file,image.toPNG());previews.push(file);
    }
    return {fixedFirst:true,pointerBlocked:true,keyboardBlocked:true,preferenceReconciled:true,noPassiveSave:true,
      selectedContentRetained:true,movableOrderSaved:true,reopened:true,generalAvailable:true,noTextSelection:true,previews,physicalInput:false};
  } finally {
    await container.invoke("preferences.set",{pageTabOrder:{aim:original}});
    await navigate(window,"/aim",".aimbot-page");
    requireHidden(window);
  }
}
