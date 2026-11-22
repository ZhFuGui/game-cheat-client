import type { BrowserWindow } from "electron";

export function requireHidden(window: BrowserWindow): void {
    if (window.isVisible() || window.isFocused())
        throw new Error("Verification requires an isolated hidden window");
}
/** Hover only in this hidden renderer. No OS input or foreground activation. */
export async function readHelp(window:BrowserWindow,label:string):Promise<string>{
    requireHidden(window);
    const name=JSON.stringify(label+" 说明");
    await window.webContents.executeJavaScript(`(()=>{const e=[...document.querySelectorAll('button')].find(e=>e.getAttribute('aria-label')===${name});if(!e)throw new Error('Help button missing');const r=e.getBoundingClientRect(),p={clientX:r.x+5,clientY:r.y+5};e.dispatchEvent(new MouseEvent('mouseenter',p));e.dispatchEvent(new MouseEvent('mousemove',{...p,bubbles:true,movementX:2}));})()`);
    await waitDom(window,`(()=>{const e=[...document.querySelectorAll('button')].find(e=>e.getAttribute('aria-label')===${name});return !!document.getElementById(e?.getAttribute('aria-describedby'));})()`,"Help did not open: "+label);
    const text=await window.webContents.executeJavaScript(`(()=>{const e=[...document.querySelectorAll('button')].find(e=>e.getAttribute('aria-label')===${name});return document.getElementById(e.getAttribute('aria-describedby')).textContent;})()`) as string;
    await window.webContents.executeJavaScript("document.dispatchEvent(new KeyboardEvent('keydown',{key:'Escape',bubbles:true}))");
    return text;
}
export async function waitDom(window: BrowserWindow, expression: string, label: string): Promise<void> {
    requireHidden(window);
    await window.webContents.executeJavaScript(`new Promise((resolve,reject)=>{
      const started=performance.now();
      const tick=()=>{if(${expression})resolve(true);else if(performance.now()-started>8000)
        reject(new Error(${JSON.stringify(label)}+' / '+document.body.innerText.slice(0,500)));
        else setTimeout(tick,15)};tick();
    })`);
}
export async function navigate(window: BrowserWindow, path: string, selector: string): Promise<void> {
    requireHidden(window);
    if(path==="/aim"||path.startsWith("/aim?")){
        const section=new URL(path,"https://mousebox.local").searchParams.get("section")??"aimbot";
        await window.webContents.executeJavaScript("document.querySelector('nav a[href=\"/aim\"]').click()");
        const tab="[data-aim-section="+JSON.stringify(section)+"]";
        await waitDom(window,`document.querySelector(${JSON.stringify(tab)})`,"Aim tabs did not mount");
        await window.webContents.executeJavaScript(`document.querySelector(${JSON.stringify(tab)}).click()`);
        await waitDom(window,`document.querySelector(${JSON.stringify(selector)})`,"Aim child did not settle: "+section);
        return;
    }
    if(path==="/?section=dashboard"){
        await navigate(window,"/",".reader-connection-panel");
        await window.webContents.executeJavaScript("document.querySelector('[data-reader-section=dashboard]').click()");
        await waitDom(window,"document.querySelector('[data-dashboard-view=summary]')","Reader dashboard children did not settle");
        await window.webContents.executeJavaScript("document.querySelector('[data-dashboard-view=summary]').click()");
        await waitDom(window,`document.querySelector(${JSON.stringify(selector)})`,"Reader dashboard did not settle");
        return;
    }
    await window.webContents.executeJavaScript(`(()=>{const link=[...document.querySelectorAll('a[href]')].find(link=>link.getAttribute('href')===${JSON.stringify(path)});if(!link)throw new Error('Missing navigation link: '+${JSON.stringify(path)});link.click()})()`);
    await waitDom(window, `document.querySelector(${JSON.stringify(selector)})`, "Page did not settle: " + path);
    if(path==="/")await waitDom(window,"document.querySelector('.reader-connection-panel')","Reader connection did not settle");
}

