import {nativeSnapshotSchema,type WorkspaceContext} from "@mousebox/shared";
import {writeFile} from "node:fs/promises";
import {dirname,join} from "node:path";
import type {SmokeContext} from "./context";
import {navigate,requireHidden,waitDom,readHelp} from "./dom";
export async function verifyTriggerbot({window,native,args,report}:Pick<SmokeContext,"window"|"native"|"args"|"report">,context:WorkspaceContext){
  requireHidden(window);
  const read=async()=>nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
  const initial=await read();
  if(initial.state.readerConnected||initial.state.boxReady)throw new Error("Triggerbot smoke requires no connected hardware");
  const set=(key:string,value:unknown)=>window.webContents.executeJavaScript("window.mousebox.invoke('settings.set',"+JSON.stringify({key,value})+","+JSON.stringify(context)+")");
  const keys=["cs2TriggerTimeOffsetMs","cs2TriggerSniperInaccuracyLimit","cs2TriggerEnabled","cs2TriggerIntervalRangeMs","cs2TriggerHoldRangeMs","cs2TriggerCoverCheck","cs2TriggerReactionEnabled","cs2TriggerReactionMs","cs2TriggerSnipers","cs2TriggerOtherGuns","aimEnabled"] as const;
  await navigate(window,"/aim?section=triggerbot",".triggerbot-page");
  await waitDom(window,"!!document.querySelector('.triggerbot-page [role=switch]')","Triggerbot controls did not mount");
  try{
    if(!(await readHelp(window,"反应时间")).includes("剩余等待最低为0ms"))throw new Error("Reaction help did not describe the effective timing");
    const offsetInput='document.querySelector(\'.triggerbot-page [aria-label="自修正时间差"] input\')';
    await waitDom(window,offsetInput+"?.value==='0'","Triggerbot timing offset did not default to zero");
    const sniperInput="document.querySelector('.trigger-weapon-types input[inputmode=decimal]')";
    await waitDom(window,sniperInput+"?.value==='0.004'","Sniper Inaccuracy limit did not default to 0.004");
    if(await window.webContents.executeJavaScript("!!document.querySelector('.triggerbot-page .hint')"))throw new Error("Triggerbot descriptions still occupy the form");
    const layout=await window.webContents.executeJavaScript(`(()=>{
      const root=document.querySelector('.trigger-categories'),conditions=root.querySelector('.trigger-conditions'),groups=[...root.querySelectorAll('.trigger-feature')];
      const rects=groups.map(e=>e.getBoundingClientRect());
      const inputCounts=groups.map(e=>e.querySelectorAll('input[inputmode]').length);
      const indented=[...root.querySelectorAll('.aim-subgroup .field')].every(field=>{
        const heading=field.closest('.aim-subgroup').querySelector(':scope > .disclosure-heading').getBoundingClientRect();
        const row=field.getBoundingClientRect();
        return row.left>heading.left&&row.top>=heading.bottom;
      });
      const columns=rects[1].right<=rects[2].left&&Math.abs(rects[1].top-rects[2].top)<1&&
        Math.abs(rects[1].width-rects[2].width)<1&&Math.abs(rects[0].left-rects[1].left)<1;
      return {switches:root.querySelectorAll('[role=switch]').length,inputs:root.querySelectorAll('input[inputmode]').length,
        grouped:JSON.stringify(inputCounts)==='[5,1,1,1]',indented,
        categories:JSON.stringify([...root.querySelectorAll('h2')].map(e=>e.textContent))==='["功能","判断条件"]'&&conditions.children.length===3,
        expanded:[...root.querySelectorAll('.disclosure-trigger')].length===6&&[...root.querySelectorAll('.disclosure-trigger')].every(e=>e.getAttribute('aria-expanded')==='true'),
        columns,
        nested:!!root.querySelector('[aria-label="掩体判断"] .trigger-reaction input:disabled'),
        removed:![...root.querySelectorAll('h2, label')].some(e=>['运行状态','检测与点击原理','参数设置','功能开关','Hitbox 内缩比例'].includes(e.textContent))};})()`);
    if(layout.switches!==5||layout.inputs!==8||!layout.nested||!layout.grouped||!layout.indented||!layout.categories||!layout.expanded||!layout.columns||!layout.removed)throw new Error("Triggerbot settings layout is incorrect: "+JSON.stringify(layout));
    await window.webContents.executeJavaScript("document.querySelectorAll('.trigger-category')[1].querySelector('.disclosure-trigger').click()");
    await waitDom(window,"document.querySelectorAll('.trigger-category')[1].querySelector(':scope > .disclosure-content').hidden","Conditions did not collapse");
    const collapsed=await read();
    if(keys.some(key=>JSON.stringify(collapsed.values[key])!==JSON.stringify(initial.values[key])))throw new Error("Collapsing conditions changed an actual setting");
    await window.webContents.executeJavaScript("document.querySelectorAll('.trigger-category')[1].querySelector('.disclosure-trigger').click()");
    await waitDom(window,"!document.querySelectorAll('.trigger-category')[1].querySelector(':scope > .disclosure-content').hidden","Conditions did not expand");
    if(!(await readHelp(window,"启用枪械类型")).includes("SCAR-20"))throw new Error("Weapon category help is missing");
    await set("cs2TriggerSnipers",false);await set("cs2TriggerOtherGuns",true);
    const weapons=await read();
    if(weapons.values.cs2TriggerSnipers!==false||weapons.values.cs2TriggerOtherGuns!==true)throw new Error("Weapon categories were not saved independently");
    await set("cs2TriggerSniperInaccuracyLimit",0.00525);
    await waitDom(window,sniperInput+"?.value==='0.00525'","Sniper Inaccuracy limit did not load the saved decimal");
    const sniperSettings=await read();
    if(sniperSettings.values.cs2TriggerSniperInaccuracyLimit!==0.00525||
        sniperSettings.values.cs2TriggerSnipers!==false||sniperSettings.values.cs2TriggerOtherGuns!==true)
      throw new Error("Sniper Inaccuracy limit did not save independently");
    await set("cs2TriggerSniperInaccuracyLimit",initial.values.cs2TriggerSniperInaccuracyLimit);
    await waitDom(window,sniperInput+"?.value==='0.004'","Sniper Inaccuracy limit did not restore");
    await set("cs2TriggerTimeOffsetMs",-5);
    await waitDom(window,offsetInput+"?.value==='-5'","Triggerbot timing offset did not render the signed value");
    const adjusted=await read();
    if(adjusted.values.cs2TriggerTimeOffsetMs!==-5||adjusted.values.cs2ResponseMs!==initial.values.cs2ResponseMs)
      throw new Error("Triggerbot timing offset was not saved independently");
    await set("cs2TriggerTimeOffsetMs",0);
    await waitDom(window,offsetInput+"?.value==='0'","Triggerbot timing offset did not restore");
    await set("cs2TriggerHoldRangeMs",{min:19,max:35,distribution:"lower"});
    await set("cs2TriggerIntervalRangeMs",{min:90,max:135,distribution:"uniform"});
    await waitDom(window,"JSON.stringify([...document.querySelectorAll('.setting-range-fields input')].map(input=>Number(input.value)))==='[90,135,19,35]'","Both click timing ranges did not render saved endpoints");
    await waitDom(window,`document.querySelector('[aria-label="连续点击最小间隔 分布模式"]')?.textContent.includes('均匀分布')&&document.querySelector('[aria-label="左键按下时长 分布模式"]')?.textContent.includes('偏向下限')`,"Click distributions did not render independently");
    await set("cs2TriggerCoverCheck",true);
    await set("cs2TriggerReactionEnabled",true);
    await waitDom(window,"document.querySelector('.trigger-reaction input:enabled')","Reaction duration did not enable under its parent");
    await set("cs2TriggerReactionMs",250);
    await set("cs2TriggerEnabled",true);
    const enabled=await read();
    if(!enabled.state.cs2Trigger?.enabled||enabled.state.cs2Trigger.clicks!==0||enabled.values.cs2RecoilCompensation!==initial.values.cs2RecoilCompensation||enabled.values.aimVersion!==initial.values.aimVersion)
      throw new Error("Triggerbot lost isolation from movement or granted unsupported output");
    if(enabled.values.cs2HitboxInsetPercent!==initial.values.cs2HitboxInsetPercent||enabled.values.cs2TriggerHoldRangeMs?.min!==19||enabled.values.cs2TriggerHoldRangeMs?.max!==35||enabled.values.cs2TriggerHoldRangeMs?.distribution!=="lower"||enabled.values.cs2TriggerIntervalRangeMs?.max!==135||enabled.values.cs2TriggerIntervalRangeMs?.distribution!=="uniform"||enabled.values.cs2TriggerCoverCheck!==true||enabled.values.cs2TriggerReactionMs!==250||enabled.values.cs2TriggerReactionEnabled!==true)
      throw new Error("Triggerbot options were not saved");
    await set("cs2TriggerCoverCheck",false);
    await waitDom(window,"document.querySelector('.trigger-reaction input:disabled')","Closing cover did not suspend the reaction setting");
    const suspended=await read();
    if(suspended.values.cs2TriggerReactionEnabled!==true||suspended.values.cs2TriggerReactionMs!==250)
      throw new Error("Cover switch changed saved reaction preferences");
    await set("cs2TriggerCoverCheck",true);
    await set("cs2TriggerEnabled",false);
    await waitDom(window,"document.querySelector('.page-context-bar[data-context-page=triggerbot]')?.textContent.includes('自动扳机已关闭')","Triggerbot disable did not update the context bar");
    await waitDom(window,"document.querySelector('.page-context-bar')?.textContent.includes('已提交点击请求 0')","Triggerbot request count is missing from the context bar");
    // Events are dispatched only into the isolated renderer, never to the OS.
    await window.webContents.executeJavaScript(`(()=>{
      const e=document.querySelector('.nav-help-trigger'),r=e.getBoundingClientRect(),p={bubbles:true,clientX:r.x+5,clientY:r.y+5};
      e.dispatchEvent(new PointerEvent('pointerover',{...p,pointerType:'mouse'}));
      e.dispatchEvent(new PointerEvent('pointermove',{...p,pointerType:'mouse'}));
      e.dispatchEvent(new MouseEvent('mouseover',p));e.dispatchEvent(new MouseEvent('mouseenter',{...p,bubbles:false}));
      e.dispatchEvent(new MouseEvent('mousemove',{...p,movementX:2}));})()`);
    await waitDom(window,"document.getElementById(document.querySelector('.nav-help-trigger').getAttribute('aria-describedby'))?.textContent.includes('每次左键按下所设时长')","Navigation help did not open on hover");
    await window.webContents.executeJavaScript("document.dispatchEvent(new KeyboardEvent('keydown',{key:'Escape',bubbles:true}))");
    await waitDom(window,"!document.querySelector('.nav-help-trigger').hasAttribute('aria-describedby')","Navigation help did not close on Escape");
    const previews:string[]=[];
    if(args.includes("--preview-triggerbot")){
      requireHidden(window);
      const [width,height]=window.getContentSize();
      await window.webContents.capturePage({x:0,y:0,width,height},{stayHidden:true,stayAwake:true});
      await window.webContents.executeJavaScript("new Promise(resolve=>setTimeout(resolve,100))");
      const capture=await window.webContents.capturePage({x:0,y:0,width,height},{stayHidden:true,stayAwake:true});
      if(capture.isEmpty())throw new Error("Triggerbot hidden preview was empty");
      const file=join(dirname(report),"triggerbot-options.png");await writeFile(file,capture.toPNG());previews.push(file);
    }
    return {page:true,layout,independent:true,disabledByDefault:true,optionsSaved:true,signedTimingOffset:true,sniperInaccuracyLimit:true,clickRanges:true,distributions:true,reactionUnderCover:true,hoverHelp:true,noDeviceOutput:true,previews};
  }finally{for(const key of keys)await set(key,initial.values[key]);}
}
