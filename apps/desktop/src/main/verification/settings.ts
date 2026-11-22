import type { SmokeContext } from "./context";
import { navigate } from "./dom";
import { verifyPreferences } from "./preferences";

export async function verifySettings(context: SmokeContext) {
    const {window}=context;
    await navigate(window, "/settings", '.settings-navigation button[data-section="input-device"]');
    const globalCategories = await window.webContents.executeJavaScript(`[...document.querySelectorAll('.settings-navigation button')].map(button=>button.dataset.section)`) as string[];
    if (globalCategories.join("|") !== "general|firmware|input-device|rendering|data")
        throw new Error("Global settings still contain game-specific panes");
    const preferenceButtons=await verifyPreferences(context);
    await navigate(window, "/game-settings", '.settings-navigation button[data-section="aimbot-algorithm"]');
    const settingsAudit = await window.webContents.executeJavaScript(`(async()=>{
      const wait=async(test)=>{for(let i=0;i<300;i++){if(test())return;await new Promise(resolve=>setTimeout(resolve,10))}throw new Error('Game settings did not settle')};
      const categories=[...document.querySelectorAll('.settings-navigation button')].map(button=>button.dataset.section);
      if(categories.join('|')!=='input|calibration|aimbot-algorithm|aimbot-mouse')throw new Error('PUBG settings categories changed unexpectedly');
      const audit={};
      for(const [section,count] of [['aimbot-algorithm',26],['aimbot-mouse',12]]){
        document.querySelector('[data-section="'+section+'"]').click();await wait(()=>document.querySelectorAll('[data-fact]').length===count);
        audit[section]={facts:[...document.querySelectorAll('[data-fact]')].map(e=>e.dataset.fact),rows:document.querySelectorAll('.speed-policy-table tbody tr').length,overflow:document.querySelector('.settings-content').scrollWidth>document.querySelector('.settings-content').clientWidth};
      }
      if(audit['aimbot-mouse'].rows!==5||audit['aimbot-algorithm'].rows!==0)throw new Error('Speed table is in the wrong game pane');
      if(new Set([...audit['aimbot-algorithm'].facts,...audit['aimbot-mouse'].facts]).size!==38)throw new Error('Algorithm facts were lost or duplicated');
      return {categories,...audit};
    })()`);
    const connectionIndicators = await window.webContents.executeJavaScript(`(()=>{
      const dots=[...document.querySelectorAll('.connection-indicator')];
      const diameter=parseFloat(getComputedStyle(dots[0],'::before').width),height=parseFloat(getComputedStyle(dots[0],'::before').height);
      const width=parseFloat(getComputedStyle(dots[0]).width),gap=parseFloat(getComputedStyle(document.querySelector('.statusbar')).gap);
      return {count:dots.length,diameter,height,visibleGap:width+gap-diameter};
    })()`) as { count: number; diameter: number; height: number; visibleGap: number };
    if (connectionIndicators.count !== 3 || Math.abs(connectionIndicators.diameter - 11.25) > .01 ||
        Math.abs(connectionIndicators.height - 11.25) > .01 || Math.abs(connectionIndicators.visibleGap - 20.4) > .01)
        throw new Error("Connection indicator geometry does not match the requested scaling");
    return { globalCategories, settingsAudit, connectionIndicators, preferenceButtons, screenshotsTaken: preferenceButtons.previews.length>0 };
}

