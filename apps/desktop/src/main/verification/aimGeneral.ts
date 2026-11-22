import {writeFile} from "node:fs/promises";
import {dirname,join} from "node:path";
import {nativeSnapshotSchema} from "@mousebox/shared";
import type {SmokeContext} from "./context";
import {navigate,requireHidden,waitDom} from "./dom";

/** Shared modules mount without writing settings or changing control ownership. */
export async function verifyAimGeneral({window,native,args,report}:Pick<SmokeContext,"window"|"native"|"args"|"report">,game:"pubg"|"cs2") {
  requireHidden(window);
  const before=nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
  const expected=game==="cs2"?["targets","runtime","solution","movement","mapping","timing"]:["solution","movement","ballistics","timing"];
  await navigate(window,"/aim?section=general",".aim-general-page");
  await waitDom(window,`JSON.stringify([...document.querySelectorAll('.aim-general-page [data-aim-module]')].map(item=>item.dataset.aimModule))===${JSON.stringify(JSON.stringify(expected))} && document.querySelectorAll('.aim-general-page .feature-section').length===${expected.length} && !!document.querySelector('.mouse-movement-algorithms [role=combobox]')`,"Shared Aim modules did not settle for "+game);
  const layout=await window.webContents.executeJavaScript(`(()=>{
    const page=document.querySelector('.aim-general-page'),grid=page.querySelector('.aim-general-modules');
    const labels=[...page.querySelectorAll('label')].map(label=>label.textContent);
    const targetOnly=['瞄准部位','默认瞄准部位','目标优先级','触发侧键','自瞄距离上限','最大瞄准角差'];
    const unboxed=[...page.querySelectorAll('.feature-section')].every(group=>getComputedStyle(group).backgroundColor==='rgba(0, 0, 0, 0)'&&parseFloat(getComputedStyle(group).borderWidth)===0);
    const normal=grid.getBoundingClientRect().width<=751;
    const sections=[...grid.children],stacked=sections.every((section,index)=>index===0||section.getBoundingClientRect().top>=sections[index-1].getBoundingClientRect().bottom);
    const saved=grid.style.maxWidth;grid.style.maxWidth='550px';
    const items=[...grid.children],narrow=items.every((item,index)=>index===0||item.getBoundingClientRect().top>=items[index-1].getBoundingClientRect().bottom);
    grid.style.maxWidth=saved;
    return {unboxed,normal,narrow,stacked,labels,manualSettingsAbsent:targetOnly.every(label=>!labels.includes(label)),
      context:document.querySelector('.page-context-bar')?.dataset.contextPage,
      fixedFirst:document.querySelector('.aim-page [role=tab]')?.dataset.tabId==='general'};
  })()`) as {unboxed:boolean;normal:boolean;narrow:boolean;stacked:boolean;labels:string[];manualSettingsAbsent:boolean;context:string;fixedFirst:boolean};
  const required=game==="cs2"?["敌人","队友","检测游戏窗口位于前台","Hitbox 内缩比例","移动轨迹","速度变化控制","跟随速度","输入换算","控制采样年龄上限","预计执行延迟"]:
    ["移动轨迹","速度变化控制","跟随速度","相机模式归零距离","重力加速度","允许采样时间差","Living / Local 补偿时间差上限"];
  if(!layout.unboxed||!layout.normal||!layout.narrow||!layout.stacked||!layout.manualSettingsAbsent||!layout.fixedFirst||layout.context!=="aim-general"||required.some(label=>!layout.labels.includes(label)))
    throw new Error("Shared Aim layout mismatch: "+JSON.stringify(layout));
  const after=nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
  if(JSON.stringify(before.values)!==JSON.stringify(after.values)||before.state.aimArmed!==after.state.aimArmed)
    throw new Error("Opening shared Aim modules changed persisted settings or armed output");
  let foregroundPolicy;
  let targetPolicy;
  let sharedInset;
  if(game==="cs2") {
    if(after.values.cs2CheckForeground!==true)throw new Error("Foreground checking must default on");
    const toggle="[...document.querySelectorAll('.aim-general-page [role=\"switch\"]')].find(item=>item.closest('.toggle-row')?.querySelector('label')?.textContent==='检测游戏窗口位于前台')";
    const click="("+toggle+").click()";
    const checked="("+toggle+").getAttribute('aria-checked')";
    await window.webContents.executeJavaScript(click);
    await waitDom(window,checked+"==='false'","Foreground opt-out did not reach the controlled switch");
    const disabled=nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
    if(disabled.values.cs2CheckForeground!==false)throw new Error("Foreground opt-out did not reach native settings");
    await window.webContents.executeJavaScript(click);
    await waitDom(window,checked+"==='true'","Foreground policy did not restore");
    const restored=nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
    if(JSON.stringify(restored.values)!==JSON.stringify(after.values))throw new Error("Foreground toggle changed another setting");
    foregroundPolicy={defaultEnabled:true,disabledThroughUi:true,restored:true,otherValuesPreserved:true};
    if(after.values.cs2RageEnemies!==true || after.values.cs2RageTeammates!==false)
      throw new Error("Shared target filter must include only enemies by default");
    for(const {label,key,initial} of [
      {label:"敌人",key:"cs2RageEnemies",initial:true},
      {label:"队友",key:"cs2RageTeammates",initial:false},
    ] as const) {
      const targetSwitch=`[...document.querySelectorAll('[data-aim-module="targets"] [role="switch"]')].find(item=>item.closest('.toggle-row')?.querySelector('label')?.textContent===${JSON.stringify(label)})`;
      const clickTarget=`(${targetSwitch}).click()`;
      const targetChecked=`(${targetSwitch})?.getAttribute('aria-checked')`;
      await waitDom(window,`${targetChecked}==='${initial}'`,"Target filter switch did not initialize: "+label);
      await window.webContents.executeJavaScript(clickTarget);
      await waitDom(window,`${targetChecked}==='${!initial}'`,"Target filter switch did not update: "+label);
      const selected=nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
      if(JSON.stringify(selected.values)!==JSON.stringify({...after.values,[key]:!initial}))
        throw new Error("Shared target edit did not persist independently: "+key);
      await window.webContents.executeJavaScript(clickTarget);
      await waitDom(window,`${targetChecked}==='${initial}'`,"Target filter switch did not restore: "+label);
      const reset=nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
      if(JSON.stringify(reset.values)!==JSON.stringify(after.values))throw new Error("Shared target settings were not restored");
    }
    targetPolicy={defaults:true,savedThroughUi:true,restored:true};
    const context=after.workspace?.active;
    const defaultInset=after.values.cs2HitboxInsetPercent;
    if(!context||defaultInset!==10)throw new Error("Shared Hitbox inset must default to 10%");
    const insetInput="document.getElementById([...document.querySelectorAll('.aim-general-page label')].find(e=>e.textContent==='Hitbox 内缩比例')?.htmlFor)";
    const setInset=async(value:number)=>{
      await window.webContents.executeJavaScript("window.mousebox.invoke('settings.set',"+
        JSON.stringify({key:"cs2HitboxInsetPercent",value})+","+JSON.stringify(context)+")");
      await waitDom(window,insetInput+`?.value==='${value}'`,"Shared inset did not render its saved value");
      const saved=nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
      if(JSON.stringify(saved.values)!==JSON.stringify({...after.values,cs2HitboxInsetPercent:value}))
        throw new Error("Shared inset did not save independently");
    };
    await waitDom(window,insetInput+"?.value==='10'","Shared inset was not shown in Aim General");
    await setInset(25.5);
    await setInset(defaultInset);
    sharedInset={defaultValue:defaultInset,edited:25.5,restored:true};
  }
  const previews:string[]=[];
  if(args.includes("--preview-aimbot")) {
    await window.webContents.capturePage(undefined,{stayHidden:true,stayAwake:true});
    await window.webContents.executeJavaScript("new Promise(resolve=>setTimeout(resolve,100))");
    const image=await window.webContents.capturePage(undefined,{stayHidden:true,stayAwake:true});
    if(image.isEmpty())throw new Error("Hidden shared Aim preview was empty");
    const file=join(dirname(report),`aim-general-${game}.png`);await writeFile(file,image.toPNG());previews.push(file);
  }
  requireHidden(window);
  return {...layout,moduleIds:expected,noPassiveWrite:true,controlOwnershipPreserved:true,foregroundPolicy,targetPolicy,sharedInset,previews,physicalOutput:false};
}
