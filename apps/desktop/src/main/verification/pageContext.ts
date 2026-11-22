import { nativeSnapshotSchema, type RuntimeState } from "@mousebox/shared";
import type { SmokeContext } from "./context";
import { navigate, waitDom, requireHidden } from "./dom";

/** Hidden DOM only: synthetic status never opens hardware or starts input capture. */
export async function verifyPageContext({ window, native, container }: SmokeContext) {
    requireHidden(window);
    const snapshot = nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
    const context = snapshot.workspace?.active ?? undefined;
    if (!context) throw new Error("Context audit needs an active native workspace");
    const navigation: string[] = [];
    for (const [path, selector, page] of [
        ["/", ".reader-page", "reader"], ["/?section=dashboard", ".data-dashboard-panel", "dashboard"], ["/visuals", ".esp-page", "esp"],
        ["/aim", ".aimbot-page", "aimbot"], ["/mouse-debug", ".mouse-debug-page", "learning"],
        ["/logs", ".logs-page", "logs"], ["/settings", '.settings-navigation button[data-section="input-device"]', "general"],
    ]) {
        await navigate(window, path, selector);
        await waitDom(window, `document.querySelector('.page-context-bar')?.dataset.contextPage===${JSON.stringify(page)}`, "Context route: " + page);
        navigation.push(page);
    }
    for (const [path, selector, cases] of [
        ["/settings", '.settings-navigation button[data-section="input-device"]', [["general", "general"], ["firmware", "firmware"], ["input-device", "input-device"], ["rendering", "rendering"], ["data", "data"]]],
        ["/game-settings", '.settings-navigation button[data-section="aimbot-algorithm"]', [["input", "input"], ["calibration", "calibration"], ["aimbot-algorithm", "algorithm"], ["aimbot-mouse", "movement"]]],
    ] as const) {
        await navigate(window, path, selector);
        for (const [section, page] of cases) {
            await window.webContents.executeJavaScript(`document.querySelector('button[data-section="${section}"]').click()`);
            await waitDom(window, `document.querySelector('.page-context-bar')?.dataset.contextPage===${JSON.stringify(page)}`, "Settings context: " + page);
            navigation.push(page);
        }
    }
    let state: RuntimeState = {
        ...snapshot.state, sessionEligible: true, readerRunning: true, readerConnected: false,
        connections: {
            reader: { phase: "error", message: "SYNTHETIC · connection timeout · ".repeat(24) },
            serial: { phase: "closed", message: "SYNTHETIC · no device" },
            box: { phase: "closed", message: "SYNTHETIC · not connected" },
        },
    };
    const publish = () => container.publish("state", state, context);
    const timer = setInterval(publish, 30);
    const waitFor = (id: string) => waitDom(window,
        `document.querySelector('.page-context-bar')?.dataset.contextId===${JSON.stringify(id)}`, "Context condition: " + id);
    const originalSize = window.getSize();
    const wasDark = await window.webContents.executeJavaScript("document.documentElement.classList.contains('dark')") as boolean;
    const layouts = [];
    try {
        publish();
        await navigate(window, "/", ".reader-page");
        await waitFor("reader:error");
        for (const dark of [false, true]) {
            await window.webContents.executeJavaScript(`document.documentElement.classList.toggle('dark',${dark})`);
            for (const [width, height] of [originalSize, [960, 640]]) {
                window.setSize(width, height);
                await new Promise(resolve => setTimeout(resolve, 80));
                requireHidden(window);
                const layout = await window.webContents.executeJavaScript(`(()=>{
                  const bar=document.querySelector('.titlebar'),context=document.querySelector('.page-context-bar'),buttons=[...bar.querySelectorAll('button')];
                  const end=buttons[buttons.length-1].getBoundingClientRect(),tolerance=1/64;
                  return {width:innerWidth,height:innerHeight,dark:document.documentElement.classList.contains('dark'),overflow:bar.scrollWidth>bar.clientWidth,
                    titleClipped:context.querySelector('.context-title').scrollWidth>context.querySelector('.context-title').clientWidth,
                    contextWidth:context.getBoundingClientRect().width,buttons:buttons.length,
                    closeVisible:end.right<=document.documentElement.getBoundingClientRect().right+tolerance&&end.left>=-tolerance};
                })()`) as { overflow: boolean; closeVisible: boolean; buttons: number; contextWidth: number; titleClipped: boolean };
                if (layout.overflow || !layout.closeVisible || layout.buttons !== 3 || layout.contextWidth <= 0 || layout.titleClipped)
                    throw new Error("Context bar overflow: " + JSON.stringify(layout));
                layouts.push(layout);
            }
        }
        await navigate(window, "/mouse-debug", ".mouse-debug-page");
        await waitFor("learning:samples");
        await window.webContents.executeJavaScript("document.querySelector('#mouse-tab-characteristics').click()");
        await waitFor("reports:ready");
        await navigate(window, "/", ".reader-page");
        await waitFor("reader:error");
        state = { ...state, readerConnected: true, connections: {
            ...state.connections!, reader: { phase: "ready", message: "SYNTHETIC · recovered" },
        } };
        publish();
        await waitFor("reader:summary");
        return { navigation, layouts, faultPreemption: true, recoveryWithdrawal: true,
            independentDiagnostics: true, synthetic: true, foregroundInteraction: false, screenshotsTaken: false };
    } finally {
        clearInterval(timer);
        container.publish("state", snapshot.state, context);
        window.setSize(originalSize[0], originalSize[1]);
        await window.webContents.executeJavaScript(`document.documentElement.classList.toggle('dark',${wasDark})`);
    }
}

