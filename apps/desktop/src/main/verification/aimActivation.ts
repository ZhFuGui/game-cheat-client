import {nativeSnapshotSchema,type WorkspaceContext} from "@mousebox/shared";
import {writeFile} from "node:fs/promises";
import {dirname,join} from "node:path";
import type {SmokeContext} from "./context";
import {navigate,requireHidden,waitDom,readHelp} from "./dom";

/** Real renderer switches and native state, isolated from sources and devices. */
export async function verifyAimActivation({window,native,args,report}:Pick<SmokeContext,"window"|"native"|"args"|"report">,context:WorkspaceContext){
  requireHidden(window);
  const read=async()=>nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
  const initial=await read();
  if(initial.state.readerConnected||initial.state.boxReady)throw Error("Aim activation requires no devices");
  const keys=["aimEnabled","cs2TriggerEnabled","cs2RageEnabled"] as const;
  const set=(key:string,value:boolean)=>window.webContents.executeJavaScript("window.mousebox.invoke('settings.set',"+JSON.stringify({key,value})+","+JSON.stringify(context)+")");
  const definitions={aimbot:{key:keys[0],label:"启用 Aimbot",page:"aimbot"},triggerbot:{key:keys[1],label:"启用自动扳机",page:"triggerbot"},rage:{key:keys[2],label:"启用 Rage",page:"rage"}};
  const click=async(name:keyof typeof definitions,enabled:boolean)=>{
    const feature=definitions[name];
    await navigate(window,`/aim?section=${name}`,`.${feature.page}-page`);
    const find=`[...document.querySelectorAll('.${feature.page}-page label')].find(label=>label.textContent===${JSON.stringify(feature.label)})?.closest('.toggle-row')?.querySelector('[role=switch]')`;
    await waitDom(window,find,"Aim switch did not mount");
    await window.webContents.executeJavaScript(`${find}.click()`);
    await waitDom(window,`${find}.getAttribute('aria-checked')==='${enabled}'`,"Aim switch did not save");
    const snapshot=await read();
    if(keys.some(key=>snapshot.values[key]!==!!(enabled&&key===feature.key))||snapshot.state.cs2Controller?.active||snapshot.state.cs2Rage?.active||snapshot.state.cs2Trigger?.clicks!==0)
      throw Error("Aim selection is not exclusive or output was granted without a device");
  };
  const previews:string[]=[];
  try{
    await navigate(window,"/aim", ".aimbot-page");
    const help=await readHelp(window,"Triggerbot");
    if(!help.includes("每次左键按下所设时长"))throw Error("Triggerbot tab help is missing");
    const placement=await window.webContents.executeJavaScript(`(()=>{
      const info=document.querySelector('.aim-page [aria-label="Triggerbot 说明"]'),slot=info.closest('.page-tab-item');
      const tab=slot?.querySelector('[role=tab]'),a=info.getBoundingClientRect(),b=tab?.getBoundingClientRect();
      info.click();
      return {owned:tab?.dataset.tabId==='triggerbot',inside:!!b&&a.left>=b.left&&a.right<=b.right&&a.top>=b.top&&a.bottom<=b.bottom,
        right:!!b&&b.right-a.right<20,noNestedButton:!info.closest('[role=tab]'),selection:document.querySelector('.aim-page [aria-selected=true]')?.dataset.tabId};
    })()`) as {owned:boolean;inside:boolean;right:boolean;noNestedButton:boolean;selection:string};
    if(!placement.owned||!placement.inside||!placement.right||!placement.noNestedButton||placement.selection!=="aimbot")throw Error("Tab help ownership or layout failed: "+JSON.stringify(placement));
    for(const name of ["triggerbot","rage","aimbot","rage","triggerbot","aimbot"] as const)await click(name,true);
    await click("aimbot",false);
    await waitDom(window,"document.querySelector('.page-context-bar')?.textContent.includes('Aimbot已关闭')","Disabled Aimbot context is missing");
    if(args.includes("--preview-aimbot")){
      await navigate(window,"/aim?section=triggerbot",".triggerbot-page");
      // The earlier Escape dismisses the same persistent trigger until the
      // pointer leaves. Re-enter it as a user would, rather than refocusing it.
      await window.webContents.executeJavaScript("(()=>{const info=document.querySelector('.aim-page [aria-label=\"Triggerbot 说明\"]');info.dispatchEvent(new MouseEvent('mouseleave',{bubbles:true}));info.blur()})()");
      await window.webContents.executeJavaScript("new Promise(resolve=>setTimeout(resolve,120))");
      await window.webContents.executeJavaScript("(()=>{const info=document.querySelector('.aim-page [aria-label=\"Triggerbot 说明\"]'),r=info.getBoundingClientRect(),p={clientX:r.x+5,clientY:r.y+5};info.dispatchEvent(new MouseEvent('mouseenter',p));info.dispatchEvent(new MouseEvent('mousemove',{...p,bubbles:true,movementX:2}))})()");
      await waitDom(window,"document.querySelector('[role=tooltip]')","Tab help did not open");
      await window.webContents.capturePage(undefined,{stayHidden:true,stayAwake:true});
      await window.webContents.executeJavaScript("new Promise(resolve=>setTimeout(resolve,100))");
      const capture=await window.webContents.capturePage(undefined,{stayHidden:true,stayAwake:true});
      const file=join(dirname(report),"aim-tab-help.png");await writeFile(file,capture.toPNG());previews.push(file);
      await window.webContents.executeJavaScript("document.dispatchEvent(new KeyboardEvent('keydown',{key:'Escape',bubbles:true}))");
    }
    return {exclusive:true,bothDirections:true,manualOff:true,allOff:true,tabHelp:placement,noDeviceOutput:true,previews};
  }finally{
    for(const key of keys)await set(key,false);
    for(const key of keys)if(initial.values[key]===true)await set(key,true);
    requireHidden(window);
  }
}
