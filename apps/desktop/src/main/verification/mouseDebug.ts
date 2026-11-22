import { readFile } from 'node:fs/promises';
import { resolve } from 'node:path';
import { mouseReportsResultSchema, mouseReportsStateSchema } from '@mousebox/shared';
import type { SmokeContext } from './context';
import { nativeSnapshotSchema } from '@mousebox/shared';
import { requireHidden } from './dom';
/** Hidden synthetic fixtures never register Raw Input or restrict the desktop cursor. */
export async function verifyMouseDebug({ window, native, container, args }: SmokeContext) {
    requireHidden(window);
    const context=nativeSnapshotSchema.parse(await native.request('runtime.snapshot')).workspace?.active??undefined;
    if(!context)throw new Error('Mouse audit needs an active native workspace');
    const publish=(value:unknown)=>container.publish('mouseReports',value,context);
    const mouseDebugAudit = await window.webContents.executeJavaScript(`(async()=>{
  document.querySelector('nav a[href="/mouse-debug"]').click();
  for(let i=0;i<200&&!document.querySelector('#mouse-tab-characteristics');i++)await new Promise(resolve=>setTimeout(resolve,10));
  const tabs=[...document.querySelectorAll('.mouse-debug-page [role=tab]')].map(e=>e.textContent);
  document.querySelector('#mouse-tab-characteristics').click();
  for(let i=0;i<200&&!document.querySelector('[data-native-surface="reports"]');i++)await new Promise(resolve=>setTimeout(resolve,10));
  const area=document.querySelector('[data-native-surface="reports"]');
  if(!area||!area.querySelector('button'))throw new Error('Report activity area or its finish button is missing');
  const bottom=[...document.querySelectorAll('.navigation-bottom a')].map(e=>e.getAttribute('href'));
  if(tabs.length!==2||bottom.join('|')!=='/mouse-debug|/logs|/settings')throw new Error('Mouse debugging navigation does not match the requested grouping');
  return {tabs,bottom,hasFinishButton:true,syntheticCharts:false};
})()`) as {
        tabs: string[];
        bottom: string[];
        hasFinishButton: boolean;
        syntheticCharts: boolean;
        captureLayout?: unknown;
    };
    await new Promise(resolve => setTimeout(resolve, 100));
    const mouseFixture = args.find(a => a.startsWith('--smoke-mouse-fixture='))?.slice('--smoke-mouse-fixture='.length);
    if (mouseFixture) {
        const fixture = mouseReportsResultSchema.parse(JSON.parse(await readFile(resolve(mouseFixture), 'utf8')));
        const actual = mouseReportsStateSchema.parse((await native.request('runtime.snapshot') as {
            mouseReports: unknown;
        }).mouseReports);
        const original = container.invoke;
        try {
            let startLayout: {
                x: number;
                y: number;
                width: number;
                height: number;
            } | undefined;
            container.invoke = async (name, args, commandContext) => {
                if (name === 'mouseReports.read')
                    return { result: fixture };
                if (name === 'mouseReports.start') {
                    startLayout = await readCaptureLayout();
                    return { ok: true };
                }
                return original(name, args, commandContext);
            };
            publish( { ...actual, hasResult: true, revision: fixture.reportId, reports: fixture.summary.reports, device: 'SYNTHETIC · ' + fixture.device, elapsedMs: fixture.summary.durationMs, reason: 'manual' });
            await window.webContents.executeJavaScript(`new Promise((resolve,reject)=>{let n=0;const timer=setInterval(()=>{if(document.querySelectorAll('.mouse-characteristics svg[role="img"]').length===3){clearInterval(timer);resolve(true)}else if(++n>100){clearInterval(timer);reject(new Error('Synthetic report charts did not render'))}},20)})`);
            const chartLayout = await window.webContents.executeJavaScript(`({overflow:document.querySelector('.mouse-characteristics').scrollWidth>document.querySelector('.mouse-characteristics').clientWidth,charts:document.querySelectorAll('.mouse-characteristics svg[role="img"]').length})`) as {
                overflow: boolean;
                charts: number;
            };
            if (chartLayout.overflow || chartLayout.charts !== 3)
                throw new Error('Report charts overflowed or were incomplete');
            const readCaptureLayout = (reset = false) => window.webContents.executeJavaScript(`(()=>{const page=document.querySelector('.mouse-debug-page');if(${reset})page.scrollTop=0;const r=document.querySelector('[data-native-surface="reports"]').getBoundingClientRect();return {x:r.x,y:r.y,width:r.width,height:r.height,scrollTop:page.scrollTop,scrollbar:page.offsetWidth-page.clientWidth}})()`);
            const originalSize = window.getSize(), layoutCases = [];
            try {
                for (const [width, height] of [originalSize, [960, 640], [1308, 864]]) {
                    window.setSize(width, height);
                    await new Promise(resolve => setTimeout(resolve, 80));
                    publish( { ...actual, hasResult: true, revision: fixture.reportId, reports: fixture.summary.reports, reason: 'manual' });
                    await window.webContents.executeJavaScript(`new Promise((resolve,reject)=>{let n=0;const timer=setInterval(()=>{if(document.querySelectorAll('.mouse-characteristics svg[role="img"]').length===3){clearInterval(timer);resolve(true)}else if(++n>100){clearInterval(timer);reject(new Error('Report layout fixture did not render'))}},20)})`);
                    const before = await readCaptureLayout(true) as {
                        x: number;
                        y: number;
                        width: number;
                        height: number;
                    };
                    // Synthetic progress only: never start Raw Input or clip the cursor.
                    publish( { ...actual, recording: true, reason: 'recording', revision: 'layout-probe', reports: 1000, elapsedMs: 1000 });
                    await window.webContents.executeJavaScript(`new Promise((resolve,reject)=>{let n=0;const timer=setInterval(()=>{if(document.querySelector('.report-activity-area.recording')){clearInterval(timer);resolve(true)}else if(++n>100){clearInterval(timer);reject(new Error('Synthetic recording state did not render'))}},20)})`);
                    const after = await readCaptureLayout() as typeof before;
                    publish( { ...actual, recording: true, reason: 'recording', revision: 'layout-probe', reports: 2000000, elapsedMs: 9999999, device: 'SYNTHETIC · full device name to check progress layout' });
                    await window.webContents.executeJavaScript(`new Promise((resolve,reject)=>{let n=0;const timer=setInterval(()=>{if(document.querySelector('.report-live-count')?.textContent.includes('2,000,000')){clearInterval(timer);resolve(true)}else if(++n>100){clearInterval(timer);reject(new Error('Synthetic report count did not render'))}},20)})`);
                    const running = await readCaptureLayout() as typeof before;
                    layoutCases.push({ window: [width, height], before, after, running });
                    if (['x', 'y', 'width', 'height'].some(key => before[key as keyof typeof before] !== after[key as keyof typeof before] || before[key as keyof typeof before] !== running[key as keyof typeof before]))
                        throw new Error('Report capture area moved when old results were hidden: ' + JSON.stringify(layoutCases));
                }
            }
            finally {
                window.setSize(originalSize[0], originalSize[1]);
            }
            await new Promise(resolve => setTimeout(resolve, 80));
            publish( { ...actual, hasResult: true, revision: fixture.reportId, reports: fixture.summary.reports, reason: 'manual' });
            await window.webContents.executeJavaScript(`new Promise((resolve,reject)=>{let n=0;const timer=setInterval(()=>{if(document.querySelectorAll('.mouse-characteristics svg[role="img"]').length===3){clearInterval(timer);resolve(true)}else if(++n>100){clearInterval(timer);reject(new Error('Scrolled result fixture did not render'))}},20)})`);
            await window.webContents.executeJavaScript(`document.querySelector('.mouse-debug-page').scrollTop=120`);
            const scrolledBefore = await readCaptureLayout();
            // Invoke the actual React start handler but intercept its native start command above.
            await window.webContents.executeJavaScript(`document.querySelector('.report-controls button').click()`);
            for (let i = 0; i < 100 && !startLayout; i++)
                await new Promise(resolve => setTimeout(resolve, 20));
            if (!startLayout)
                throw new Error('Prepared capture start did not arrive');
            publish( { ...actual, recording: true, reason: 'recording', revision: 'layout-start-probe' });
            await window.webContents.executeJavaScript(`new Promise((resolve,reject)=>{let n=0;const timer=setInterval(()=>{if(document.querySelector('.report-activity-area.recording')){clearInterval(timer);resolve(true)}else if(++n>100){clearInterval(timer);reject(new Error('Prepared recording state did not render'))}},20)})`);
            const scrolledAfter = await readCaptureLayout() as typeof startLayout;
            if (['x', 'y', 'width', 'height'].some(key => startLayout![key as keyof typeof startLayout] !== scrolledAfter[key as keyof typeof startLayout]))
                throw new Error('Capture area moved after the prepared start: ' + JSON.stringify({ scrolledBefore, startLayout, scrolledAfter }));
            mouseDebugAudit.captureLayout = { cases: layoutCases, scrolledStart: { before: scrolledBefore, atStart: startLayout, after: scrolledAfter }, nativeStartIntercepted: true };
            // A completed real capture has a new revision; never replay the suppressed old result.
            fixture.reportId = 'layout-complete';
            publish( { ...actual, hasResult: true, revision: fixture.reportId, reports: fixture.summary.reports, device: 'SYNTHETIC · ' + fixture.device, elapsedMs: fixture.summary.durationMs, reason: 'manual' });
            await window.webContents.executeJavaScript(`new Promise((resolve,reject)=>{let n=0;const timer=setInterval(()=>{if(document.querySelectorAll('.mouse-characteristics svg[role="img"]').length===3){clearInterval(timer);resolve(true)}else if(++n>100){clearInterval(timer);reject(new Error('Synthetic report charts did not return'))}},20)})`);
            await window.webContents.executeJavaScript(`document.querySelector('.report-results-heading').scrollIntoView({block:'start'})`);
            await new Promise(resolve => setTimeout(resolve, 100));
            await window.webContents.executeJavaScript(`document.querySelector('.report-distributions').scrollIntoView({block:'start'})`);
            await new Promise(resolve => setTimeout(resolve, 100));
            mouseDebugAudit.syntheticCharts = true;
        }
        finally {
            container.invoke = original;
            publish( actual);
        }
    }
    return mouseDebugAudit;
}
