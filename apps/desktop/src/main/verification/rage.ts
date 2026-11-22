import {nativeSnapshotSchema,type WorkspaceContext} from "@mousebox/shared";
import {writeFile} from "node:fs/promises";
import {dirname,join} from "node:path";
import type {SmokeContext} from "./context";
import {navigate,requireHidden,waitDom,readHelp} from "./dom";

export async function verifyRage({window,native,args,report}:Pick<SmokeContext,"window"|"native"|"args"|"report">,context:WorkspaceContext){
  requireHidden(window);
  const read=async()=>nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
  const initial=await read();
  if(initial.state.readerConnected||initial.state.boxReady)throw Error("Rage verification requires isolated no-device mode");
  const set=(key:string,value:unknown)=>window.webContents.executeJavaScript("window.mousebox.invoke('settings.set',"+JSON.stringify({key,value})+","+JSON.stringify(context)+")");
  const keys=["aimEnabled","cs2RageEnabled","cs2RageEnemies","cs2RageTeammates","cs2RageCoverCheck","cs2RageOtherPlayerCheck","cs2RageThreatWeight","cs2TriggerEnabled"] as const;
  try{
    await navigate(window,"/aim?section=rage",".rage-page");
    await waitDom(window,"document.querySelector('.rage-page [role=switch]')","Rage did not mount");
    const stacked=await window.webContents.executeJavaScript("[...document.querySelector('.rage-layout').children].every((group,index,groups)=>index===0||group.getBoundingClientRect().top>=groups[index-1].getBoundingClientRect().bottom)") as boolean;
    if(!stacked)throw Error('Rage H2 groups must be vertically stacked');
    if(initial.values.cs2RageEnabled!==false)throw Error("Fresh Rage setting must be disabled");
    if(!(await readHelp(window,"威胁权重")).includes("对方武器预计伤害"))throw Error("Threat estimate explanation is missing");
    if(initial.values.cs2RageScoreModel!=='threat-v2'||initial.values.cs2RageSwitchMargin!==1)
      throw Error('Rage scoring defaults were not migrated');
    if(initial.values.cs2RageCoverCheck!==true)throw Error('Rage cover check must preserve the previous default');
    await waitDom(window,"[...document.querySelectorAll('.rage-page label')].find(e=>e.textContent==='掩体判断')?.closest('.toggle-row')?.querySelector('[role=switch]')?.getAttribute('aria-checked')==='true'",'Rage cover switch did not initialize');
    const coverLayout=await window.webContents.executeJavaScript(`(()=>{
      const group=document.querySelector('.rage-page [role="group"][aria-label="判断条件"]');
      const label=[...document.querySelectorAll('.rage-page label')].find(e=>e.textContent==='掩体判断');
      const control=label?.closest('.toggle-row')?.querySelector('[role=switch]');
      const moved=![...document.querySelectorAll('.rage-page label')].some(item=>['敌人','队友','Hitbox 内缩比例'].includes(item.textContent));
      return {inConditions:group?.contains(control), checked:control?.getAttribute('aria-checked'), moved};
    })()`) as {inConditions:boolean;checked:string;moved:boolean};
    if(!coverLayout.inConditions||coverLayout.checked!=='true'||!coverLayout.moved)
      throw Error('Rage cover or shared target settings are misplaced: '+JSON.stringify(coverLayout));
    await set('cs2RageCoverCheck',false);
    if((await read()).values.cs2RageCoverCheck!==false)throw Error('Rage cover switch did not save');
    await set('cs2RageCoverCheck',true);
    if(initial.values.cs2RageOtherPlayerCheck!==true)throw Error('Other-player check must preserve the previous default');
    await waitDom(window,"[...document.querySelectorAll('.rage-page label')].find(e=>e.textContent==='其他人物遮挡判断')?.closest('.toggle-row')?.querySelector('[role=switch]')?.getAttribute('aria-checked')==='true'",'Other-player switch did not initialize');
    const playerPeer=await window.webContents.executeJavaScript(`!![...document.querySelectorAll('.rage-page [role="group"][aria-label="判断条件"] label')].find(e=>e.textContent==='其他人物遮挡判断')`) as boolean;
    if(!playerPeer)throw Error('Other-player switch is misplaced');
    await set('cs2RageOtherPlayerCheck',false);
    if((await read()).values.cs2RageOtherPlayerCheck!==false)throw Error('Other-player switch did not save');
    await set('cs2RageOtherPlayerCheck',true);
    await waitDom(window,"document.querySelector('.rage-page')?.textContent.includes('评分方案') && document.querySelector('.rage-score-details') && [...document.querySelectorAll('.rage-page input')].some(input=>input.labels?.[0]?.textContent==='目标切换分差'&&input.value==='1')",'Rage scoring controls or one-point margin are missing');
    await waitDom(window,"document.querySelector('.rage-score-formulas')?.textContent.includes('综合分 = 70% × 威胁分 + 30% × 我方伤害分') && document.querySelector('.rage-score-formulas')?.textContent.includes('> 1分 → 立即切换')",'Visible Rage scoring formulas are missing');
    await set('cs2RageEnemies',false);await set('cs2RageTeammates',false);
    await set("cs2TriggerEnabled",true);await set("cs2RageEnabled",true);
    const enabled=await read();
    if(!enabled.state.cs2Rage?.enabled||enabled.state.cs2Rage.active||enabled.state.cs2Rage.clicks!==0||
      enabled.values.aimEnabled||enabled.values.cs2RecoilCompensation!==initial.values.cs2RecoilCompensation||enabled.values.cs2TriggerEnabled||enabled.state.cs2Controller?.active)
      throw Error("Rage did not revoke competing output or granted output without a source");
    await waitDom(window,"document.querySelector('.page-context-bar[data-context-page=rage]')?.textContent.includes('未选择包含对象')",'No-target reason did not settle');
    await set('cs2RageEnemies',true);
    await set("cs2RageThreatWeight",70);
    const configured=await read();
    if(configured.values.cs2HitboxInsetPercent!==initial.values.cs2HitboxInsetPercent)throw Error("Rage changed the shared inset");
    await set("cs2RageEnabled",false);
    await waitDom(window,"document.querySelector('.page-context-bar[data-context-page=rage]')?.textContent.includes('Rage已关闭')","Rage context did not settle");
    const previews:string[]=[];
    if(args.includes("--preview-rage")){
      const [width,height]=window.getContentSize();
      await window.webContents.capturePage({x:0,y:0,width,height},{stayHidden:true,stayAwake:true});
      await window.webContents.executeJavaScript("new Promise(resolve=>setTimeout(resolve,100))");
      const capture=await window.webContents.capturePage({x:0,y:0,width,height},{stayHidden:true,stayAwake:true});
      if(capture.isEmpty())throw Error("Hidden Rage preview was empty");
      const path=join(dirname(report),"rage.png");await writeFile(path,capture.toPNG());previews.push(path);
      await window.webContents.executeJavaScript("(()=>{const p=document.querySelector('.rage-score-formulas');p.style.scrollMarginTop='60px';p.scrollIntoView({block:'start',behavior:'instant'})})()");
      await window.webContents.capturePage({x:0,y:0,width,height},{stayHidden:true,stayAwake:true});
      await window.webContents.executeJavaScript("new Promise(resolve=>setTimeout(resolve,100))");
      await waitDom(window,"(()=>{const p=document.querySelector('.rage-score-formulas'),r=p.getBoundingClientRect();return r.top>=0&&r.top<innerHeight/2&&p.scrollWidth<=p.clientWidth+1})()",'Scoring formulas did not scroll into view or overflowed');
      const formulaCapture=await window.webContents.capturePage({x:0,y:0,width,height},{stayHidden:true,stayAwake:true});
      if(formulaCapture.isEmpty())throw Error('Hidden scoring formula preview was empty');
      const formulaPath=join(dirname(report),'rage-formulas.png');await writeFile(formulaPath,formulaCapture.toPNG());previews.push(formulaPath);
      await window.webContents.executeJavaScript("document.querySelector('.rage-score-formulas').style.removeProperty('scroll-margin-top')");
    }
    return {page:true,stacked,disabledByDefault:true,exclusive:true,settingsSaved:true,sharedTargetsMoved:coverLayout.moved,scoringControls:true,visibleFormulas:true,onePointMargin:true,noDeviceOutput:true,previews};
  }finally{for(const key of keys)await set(key,initial.values[key]);}
}
