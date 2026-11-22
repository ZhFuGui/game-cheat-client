import {writeFile} from "node:fs/promises";
import {dirname, join} from "node:path";
import type {BrowserWindow} from "electron";
import {nativeSnapshotSchema} from "@mousebox/shared";
import type {SmokeContext} from "./context";
import {requireHidden, waitDom} from "./dom";
import {verifyWeaponAimSettings} from "./weaponAim";

/** Check the real Aimbot renderer without connecting a source or a device. */
export async function verifyAimbotLayout(window:BrowserWindow, args:string[], report:string, game:"pubg"|"cs2", native:SmokeContext["native"]) {
    requireHidden(window);
    const groupsExpected=game==="cs2"?3:2;
    await waitDom(window, "document.querySelectorAll('.aimbot-page .feature-section').length===" + groupsExpected,
        "Aimbot groups did not finish mounting");
    const title=game==='cs2'?'CS2 跟随设置':'跟随设置';
    await waitDom(window,`[...document.querySelectorAll('.aimbot-page h2')].some(title=>title.textContent===${JSON.stringify(title)})`,
        'Aimbot game content did not settle: '+game);
    const layout = await window.webContents.executeJavaScript(`(()=>{
      const page=document.querySelector('.aimbot-page');
      const groups=[...page.querySelectorAll('.feature-section')];
      const settings=page.querySelector('.aimbot-settings');
      const sharedRemoved=!page.querySelector('.mouse-movement-algorithms,.aim-version-details') &&
        ![...page.querySelectorAll('label')].some(label=>['跟随速度','输入换算','控制采样年龄上限','预计执行延迟','重力加速度','允许采样时间差'].includes(label.textContent));
      const runtimeRemoved=!page.querySelector('.live-control-status,.aimbot-columns') &&
        !['运行控制','Runtime controls','链路就绪后自动启用','Enabled automatically when connected'].some(text=>page.textContent.includes(text));
      const contextRetained=document.querySelector('.page-context-bar')?.dataset.contextPage==='aimbot';
      const unboxed=groups.every(group=>{
        const style=getComputedStyle(group);
        return style.backgroundColor==='rgba(0, 0, 0, 0)' && parseFloat(style.borderWidth)===0;
      });
      const titles=groups.every(group=>getComputedStyle(group.querySelector('h2')).fontSize==='18px');
      const normal=settings?.getBoundingClientRect().width<=751 && settings.children.length===${groupsExpected};
      const stacked=groups.every((group,index)=>index===0||group.getBoundingClientRect().top>=groups[index-1].getBoundingClientRect().bottom);
      return {unboxed,titles,normal,stacked,sharedRemoved,runtimeRemoved,contextRetained,oldCards:page.querySelectorAll('.section').length};
    })()`) as {unboxed:boolean;titles:boolean;normal:boolean;stacked:boolean;sharedRemoved:boolean;runtimeRemoved:boolean;contextRetained:boolean;oldCards:number};
    if(!layout.unboxed||!layout.titles||!layout.normal||!layout.stacked||!layout.sharedRemoved||!layout.runtimeRemoved||!layout.contextRetained||layout.oldCards)
        throw new Error("Aimbot layout failed: "+JSON.stringify(layout));
    let coverPolicy;
    if(game==="cs2") {
        const before=nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
        if(before.values.cs2AimCoverCheck!==false)throw new Error("Manual aim cover must default off");
        const toggle="[...document.querySelectorAll('.aimbot-page [role=\"switch\"]')].find(item=>item.closest('.toggle-row')?.querySelector('label')?.textContent==='掩体判断')";
        const click="("+toggle+").click()",checked="("+toggle+").getAttribute('aria-checked')";
        await window.webContents.executeJavaScript(click);
        await waitDom(window,checked+"==='true'","Manual aim cover did not enable through its real switch");
        const enabled=nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
        if(enabled.values.cs2AimCoverCheck!==true)throw new Error("Manual aim cover did not reach native settings");
        if(JSON.stringify({...enabled.values,cs2AimCoverCheck:false})!==JSON.stringify(before.values))
            throw new Error("Manual aim cover changed another setting");
        await window.webContents.executeJavaScript(click);
        await waitDom(window,checked+"==='false'","Manual aim cover did not restore through its real switch");
        const restored=nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
        if(JSON.stringify(restored.values)!==JSON.stringify(before.values))throw new Error("Manual aim cover did not restore native settings");
        coverPolicy={defaultDisabled:true,enabledThroughUi:true,restored:true,otherValuesPreserved:true};
    }
    const weaponParts=game==="cs2" ? await verifyWeaponAimSettings(window,native) : undefined;
    const previews:string[]=[];
    if(args.includes("--preview-aimbot")) {
        await window.webContents.capturePage(undefined,{stayHidden:true,stayAwake:true});
        await window.webContents.executeJavaScript('new Promise(resolve=>setTimeout(resolve,100))');
        const image=await window.webContents.capturePage(undefined,{stayHidden:true,stayAwake:true});
        if(image.isEmpty())throw new Error("Hidden Aimbot preview was empty");
        const file=join(dirname(report),`aimbot-${game}.png`);
        await writeFile(file,image.toPNG());
        previews.push(file);
    }
    requireHidden(window);
    return {...layout,coverPolicy,weaponParts,previews};
}
