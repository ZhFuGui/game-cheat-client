import { nativeSnapshotSchema } from "@mousebox/shared";
import type { SmokeContext } from "./context";
import { waitDom, requireHidden,readHelp } from "./dom";

/** DOM/layout responsiveness only; no foreground paint latency is claimed. */
export async function verifyNavigation({ window, container, version }: SmokeContext, bootstrap: unknown) {
    requireHidden(window);
    const expected = ["/", "/visuals", "/aim", "/game-settings", "/mouse-debug", "/logs", "/settings"];
    await waitDom(window, `!document.querySelector('.startup-state') && ${JSON.stringify(expected)}.every(path=>document.querySelector('nav a[href="'+path+'"]'))`, "Web shell did not become ready");
    const renderer = await window.webContents.executeJavaScript(`window.mousebox.invoke('bootstrap', {}).then(value => ({
      bridge:true,version:value.version,workspace:value.workspace?.active,
      links:[...document.querySelectorAll('nav a')].map(link=>link.getAttribute('href')),
      header:(()=>{
        const header=document.querySelector('.sidebar-header'),selector=header?.querySelector('[role=combobox]'),version=header?.querySelector('.sidebar-version');
        if(!selector||!version)return null;
        const a=selector.getBoundingClientRect(),b=version.getBoundingClientRect();
        return {game:selector.textContent,version:version.textContent,projectNameRemoved:!header.textContent.includes('game-cheat-client'),
          captionRemoved:!document.querySelector('.navigation-caption'),sameRow:Math.abs(a.top+a.height/2-b.top-b.height/2)<1,
          noOverlap:a.right<=b.left,menuTrigger:selector.classList.contains('ui-select-menu')};
      })(),
      innerWidth,innerHeight,pixelRatio:devicePixelRatio,error:document.querySelector('[role=alert]')?.textContent||null
    }))`) as { bridge: boolean; version: string; workspace: unknown; header: null | {
        game: string; version: string; projectNameRemoved: boolean; captionRemoved: boolean;
        sameRow: boolean; noOverlap: boolean; menuTrigger: boolean;
    }; links: string[] };
    if (!renderer.bridge || renderer.version !== version || renderer.header?.version !== `v${version}`)
        throw new Error("Renderer preload/IPC or version handshake failed");
    if (!renderer.header.projectNameRemoved || !renderer.header.captionRemoved || !renderer.header.sameRow ||
        !renderer.header.noOverlap || !renderer.header.menuTrigger)
        throw new Error("Compact game/version header layout did not match its contract");
    if (renderer.links.includes("/esp") || renderer.links.includes("/radar"))
        throw new Error("A Visuals child has a separate sidebar link");
    const snapshot = nativeSnapshotSchema.parse(bootstrap);
    const context = snapshot.workspace?.active ?? undefined;
    if (!context) throw new Error("Navigation verification needs the real PUBG workspace");
    const syntheticStatus = setInterval(() => container.publish("state", { ...snapshot.state, characters: 200 }, context), 100);
    let uiPerformance: unknown;
    try {
        uiPerformance = await window.webContents.executeJavaScript(`(async()=>{
          const pages=[['/?section=dashboard','.data-dashboard-panel'],['/aim','.aimbot-page'],['/aim?section=triggerbot','.triggerbot-page'],['/mouse-debug','.mouse-debug-page'],['/settings','.settings-navigation button[data-section="input-device"]'],['/game-settings','.settings-navigation button[data-section="aimbot-algorithm"]'],['/logs','.logs-page'],['/visuals','.esp-page'],['/','.reader-connection-panel']];
          const measurements=[];let maxLogRows=0;
          const settle=selector=>new Promise((resolve,reject)=>{let tries=0;const tick=()=>{const page=document.querySelector(selector);if(page){page.getBoundingClientRect();resolve(true)}else if(++tries>500)reject(new Error('Page did not render: '+selector+' / '+document.body.innerText.slice(0,800)));else setTimeout(tick,5)};tick()});
          for(let cycle=0;cycle<5;cycle++)for(const [path,selector]of pages){
            const start=performance.now(),url=new URL(path,'https://mousebox.local');document.querySelector('nav a[href="'+url.pathname+'"]').click();
            if(path==='/?section=dashboard'){await settle('[data-reader-section=dashboard]');document.querySelector('[data-reader-section=dashboard]').click();}
            if(url.pathname==='/aim'){const section=url.searchParams.get('section')||'aimbot',tab='[data-aim-section="'+section+'"]';await settle(tab);document.querySelector(tab).click();}
            await settle(selector);
            if(cycle)measurements.push(performance.now()-start);
            maxLogRows=Math.max(maxLogRows,document.querySelectorAll('.log-row').length);
          }
          measurements.sort((a,b)=>a-b);
          return {kind:'hidden-electron-dom-layout-navigation',samples:measurements.length,p95Ms:measurements[Math.ceil(measurements.length*0.95)-1],maximumMs:Math.max(...measurements),maxLogRows,foregroundInteraction:false,paintLatencyMeasured:false};
        })()`);
    } finally { clearInterval(syntheticStatus); container.publish("state", snapshot.state, context); }
    if(await window.webContents.executeJavaScript("!!document.querySelector('nav a[href=\"/dashboard\"]')"))throw new Error("Dashboard retained its removed sidebar entry");
    await waitDom(window,"document.querySelector('.reader-chain h2')?.textContent==='订阅链路耗时'","Reader-only timing panel did not load");
    if(!(await readHelp(window,"订阅链路耗时")).includes("近2秒平均"))throw new Error("Reader timing explanation is missing");
    return { renderer, uiPerformance, readerChainPanel:true };
}

