import {writeFile} from "node:fs/promises";
import {dirname,join} from "node:path";
import {bootstrapSchema} from "@mousebox/shared";
import type {SmokeContext} from "./context";
import {navigate,requireHidden,waitDom} from "./dom";
import {verifyTabReorder} from "./tabReorder";

async function verifyVisualsTabs(window: SmokeContext["window"]) {
  await navigate(window, "/visuals", ".visuals-esp-panel .esp-page");
  const layout = await window.webContents.executeJavaScript(`(() => {
    const page = document.querySelector('.visuals-page');
    const tabs = [...page.querySelectorAll('[role=tab]')];
    const panel = page.querySelector('[role=tabpanel]').getBoundingClientRect();
    const canvas = page.querySelector('[data-native-surface=esp]').getBoundingClientRect();
    const workspace = document.querySelector('.workspace').getBoundingClientRect();
    return { tabs: tabs.map(tab => tab.dataset.tabId),
      defaultTab: tabs.find(tab => tab.getAttribute('aria-selected') === 'true')?.dataset.tabId,
      generalFixed: !tabs[0]?.hasAttribute('data-reorderable'),
      canvasFits: canvas.height >= 150 && canvas.top > panel.top && canvas.bottom < panel.bottom &&
        canvas.left >= workspace.left && canvas.right <= workspace.right && panel.bottom <= workspace.bottom + 1 };
  })()`) as { tabs: string[]; defaultTab: string; generalFixed: boolean; canvasFits: boolean };
  if (layout.tabs.join(',') !== 'general,esp,radar' || layout.defaultTab !== 'esp' || !layout.generalFixed || !layout.canvasFits)
    throw new Error("Visuals tabs or canvas layout mismatch: " + JSON.stringify(layout));
  for (const [tab, context] of [["general", "visuals-general"], ["radar", "radar"]] as const) {
    await window.webContents.executeJavaScript(`document.getElementById('visuals-tab-${tab}').click()`);
    await waitDom(window, `document.getElementById('visuals-panel-${tab}')?.children.length === 0 &&
      !document.querySelector('[data-native-surface=esp]') &&
      document.querySelector('.page-context-bar')?.dataset.contextPage === '${context}' &&
      document.querySelector('nav a[href="/visuals"]')?.getAttribute('aria-current') === 'page'`,
    "Visuals placeholder did not settle: " + tab);
  }
  await window.webContents.executeJavaScript("document.getElementById('visuals-tab-esp').click()");
  await waitDom(window, "!!document.querySelector('#visuals-panel-esp [data-native-surface=esp]') && document.querySelector('.page-context-bar')?.dataset.contextPage === 'esp'",
    "Returning to ESP did not restore the canvas and page context");
  return { ...layout, placeholderTabs: true, restoredEsp: true };
}

/** Only this isolated renderer's backing surface is captured, with stayHidden. */
export async function verifyPageTabs(context:SmokeContext){
  const {window,container,args,report}=context;
  requireHidden(window);
  await navigate(window,"/",".reader-connection-panel");
  await window.webContents.executeJavaScript("document.querySelector('.reader-page').scrollTop=0");
  const layout=await window.webContents.executeJavaScript(`(()=>{
    const workspace=document.querySelector('.workspace').getBoundingClientRect();
    const strip=document.querySelector('.reader-page > .page-tabs').getBoundingClientRect();
    const selected=document.querySelector('.page-tabs [aria-selected=true]');
    const tab=selected.getBoundingClientRect(),panel=document.querySelector('.browser-page-content');
    const content=panel.getBoundingClientRect();
    return {top:Math.abs(strip.top-workspace.top),width:Math.abs(strip.width-workspace.width),
      connected:Math.abs(tab.bottom-content.top),pageHeadingRemoved:!panel.querySelector('h1,.page-header'),
      rounded:parseFloat(getComputedStyle(selected).borderTopLeftRadius)>=8,
      sameSurface:getComputedStyle(selected).backgroundColor===getComputedStyle(panel).backgroundColor,
      tabColor:getComputedStyle(selected).backgroundColor,panelColor:getComputedStyle(panel).backgroundColor};
  })()`) as {top:number;width:number;connected:number;pageHeadingRemoved:boolean;rounded:boolean;sameSurface:boolean};
  if(layout.top>1||layout.width>1||layout.connected>1||!layout.pageHeadingRemoved||!layout.rounded||!layout.sameSurface)
    throw new Error("Reader browser-tab layout mismatch: "+JSON.stringify(layout));
  await navigate(window,"/?section=dashboard",".data-dashboard-panel");
  const contentNavigation=await window.webContents.executeJavaScript(`(()=>{
    const page=document.querySelector('.reader-page');page.scrollTop=0;
    const bars=[...page.querySelectorAll('.page-tabs')];
    const buttons=[...page.querySelectorAll('[data-dashboard-view]')];
    return {levels:bars.length,contentButtons:buttons.length,buttonsOnly:buttons.every(button=>button.tagName==='BUTTON'&&button.hasAttribute('aria-pressed')),
      connected:bars.every(bar=>{const tab=bar.querySelector('[aria-selected=true]'),panel=document.getElementById(tab.getAttribute('aria-controls'));
        return Math.abs(tab.getBoundingClientRect().bottom-panel.getBoundingClientRect().top)<1&&getComputedStyle(tab).backgroundColor===getComputedStyle(panel).backgroundColor}),
      browserStyle:bars.every(bar=>!bar.classList.contains('tabs')),pageHeadingRemoved:!page.querySelector('h1,.page-header')};
  })()`) as {levels:number;contentButtons:number;buttonsOnly:boolean;connected:boolean;browserStyle:boolean;pageHeadingRemoved:boolean};
  if(contentNavigation.levels!==1||contentNavigation.contentButtons!==2||!contentNavigation.buttonsOnly||!contentNavigation.connected||!contentNavigation.browserStyle||!contentNavigation.pageHeadingRemoved)
    throw new Error("Single-level page navigation failed: "+JSON.stringify(contentNavigation));
  const reorder=await verifyTabReorder(context);
  await navigate(window,"/?section=dashboard",".data-dashboard-panel");
  const previews:string[]=[];
  if(args.includes('--page-tab-previews')){
    const original=bootstrapSchema.parse(await container.invoke('bootstrap',{})).preferences;
    try{
      for(const mode of ['light','dark'] as const){
        await container.invoke('preferences.set',{colorMode:mode});
        await waitDom(window,`document.documentElement.classList.contains('dark')===${mode==='dark'}`,"Tab preview theme did not settle");
        await window.webContents.executeJavaScript("new Promise(resolve=>setTimeout(resolve,120))");
        const [width,height]=window.getContentSize();
        requireHidden(window);
        // Prime the hidden compositor after route/theme updates; the first
        // copy may still contain the previous backing frame.
        await window.webContents.capturePage({x:0,y:0,width,height:Math.min(height,650)},{stayHidden:true,stayAwake:true});
        await window.webContents.executeJavaScript("new Promise(resolve=>setTimeout(resolve,80))");
        const capture=await window.webContents.capturePage({x:0,y:0,width,height:Math.min(height,650)},{stayHidden:true,stayAwake:true});
        if(capture.isEmpty())throw new Error("Own renderer tab preview was empty");
        const file=join(dirname(report),`reader-tabs-${mode}.png`);
        await writeFile(file,capture.toPNG());previews.push(file);
        requireHidden(window);
      }
    }finally{await container.invoke('preferences.set',{colorMode:original.colorMode});}
  }
  const visuals = await verifyVisualsTabs(window);
  await navigate(window,"/",".reader-connection-panel");
  return {browserTabs:true,layout,contentNavigation,reorder,visuals,previews,ownRendererOnly:true,desktopCapture:false};
}
