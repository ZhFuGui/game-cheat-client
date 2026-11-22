import type {BrowserWindow} from "electron";
import {nativeSnapshotSchema} from "@mousebox/shared";
import type {SmokeContext} from "./context";
import {requireHidden, waitDom} from "./dom";

/** Exercise the actual controls in an isolated renderer; no OS input is sent. */
export async function verifyWeaponAimSettings(window: BrowserWindow, native: SmokeContext["native"]) {
  requireHidden(window);
  const read = async()=>nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
  const initial = await read();
  const hitboxDefaults = {cs2AimPointIntervalMs:150,cs2AimPointDistribution:"uniform"} as const;
  if(Object.entries(hitboxDefaults).some(([key,value])=>initial.values[key as keyof typeof hitboxDefaults]!==value))
    throw new Error("Incorrect Hitbox aim sampling defaults");
  await waitDom(window,"![...document.querySelectorAll('.aimbot-page label')].some(e=>e.textContent==='Hitbox 内缩比例') && [...document.querySelectorAll('.aimbot-page label')].some(e=>e.textContent==='瞄点切换间隔' && document.getElementById(e.htmlFor)?.value==='150')","Hitbox sampling fields were misplaced");
  const sampling="[...document.querySelectorAll('.aimbot-page [role=combobox]')].find(e=>e.getAttribute('aria-label')==='瞄点分布')";
  await waitDom(window,"("+sampling+")?.textContent.includes('体积均匀')","Hitbox sampling selector did not show its default");
  const choices = [
    {key:"cs2AimPartPistol", label:"手枪", original:0, text:"头部", test:1, selected:"颈部"},
    {key:"cs2AimPartRifle", label:"步枪", original:1, text:"颈部", test:3, selected:"腹部"},
    {key:"cs2AimPartSmg", label:"冲锋枪", original:1, text:"颈部", test:2, selected:"胸部"},
    {key:"cs2AimPartSniper", label:"狙击枪", original:2, text:"胸部", test:-1, selected:"沿用默认瞄准部位"},
  ] as const;
  for(const choice of choices) {
    if(initial.values[choice.key]!==choice.original)throw new Error("Incorrect weapon-category default: "+choice.key);
    const control=`[...document.querySelectorAll('.weapon-aim-parts [role="combobox"]')].find(e=>e.getAttribute('aria-label')===${JSON.stringify(choice.label)})`;
    await waitDom(window,`(${control})?.textContent.includes(${JSON.stringify(choice.text)})`,"Missing weapon-category choice: "+choice.label);
    const select=async(text:string)=>{
      await window.webContents.executeJavaScript(`(${control}).click()`);
      const popup=`document.getElementById((${control}).getAttribute('aria-controls'))`;
      const option=`[...(${popup})?.querySelectorAll('[role="option"]')??[]].find(e=>e.textContent===${JSON.stringify(text)})`;
      await waitDom(window,`!!(${option})`,"Weapon aim option did not open: "+text);
      await window.webContents.executeJavaScript(`(()=>{const option=${option};
        option.dispatchEvent(new PointerEvent('pointerdown',{bubbles:true,pointerType:'touch'}));option.click();})()`);
      await waitDom(window,`(${control})?.textContent.includes(${JSON.stringify(text)})`,"Weapon aim choice did not settle: "+text);
      await waitDom(window,`(${control})?.getAttribute('aria-expanded')!=='true'`,"Weapon aim popup did not close");
    };
    await select(choice.selected);
    const saved = await read();
    if(JSON.stringify(saved.values)!==JSON.stringify({...initial.values,[choice.key]:choice.test}))
      throw new Error("Category edit changed another setting or did not save: "+choice.key);
    await select(choice.text);
    if(JSON.stringify((await read()).values)!==JSON.stringify(initial.values))
      throw new Error("Weapon-category choice did not restore: "+choice.key);
  }
  await window.webContents.executeJavaScript("document.querySelector('.weapon-aim-parts .disclosure-trigger').click()");
  await waitDom(window,"document.querySelector('.weapon-aim-parts > .disclosure-content').hidden","Weapon categories did not collapse");
  if(JSON.stringify((await read()).values)!==JSON.stringify(initial.values))throw new Error("Collapsing weapon choices changed settings");
  await window.webContents.executeJavaScript("document.querySelector('.weapon-aim-parts .disclosure-trigger').click()");
  await waitDom(window,"!document.querySelector('.weapon-aim-parts > .disclosure-content').hidden","Weapon categories did not expand");
  requireHidden(window);
  return {defaults:true,categorySaved:true,inheritedSaved:true,restored:true,otherValuesPreserved:true,collapsePreserved:true,hitboxSampling:true};
}
