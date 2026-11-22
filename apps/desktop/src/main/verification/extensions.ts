import { cs2CoverEnabledSetting, cs2CoverExtensionId, cs2RemainingDamageEnabledSetting, cs2RemainingDamageExtensionId,
    cs2HeadDamageEnabledSetting, cs2HeadDamageExtensionId, cs2BestDamageEnabledSetting, cs2BestDamageExtensionId, nativeSnapshotSchema, type WorkspaceContext } from "@mousebox/shared";
import type { SmokeContext } from "./context";
import { requireHidden, waitDom,readHelp } from "./dom";

/** Real hidden panel/IPC; sources remain disconnected and no geometry is loaded. */
export async function verifyExtensions({ window, native }: Pick<SmokeContext, "window" | "native">, context: WorkspaceContext) {
    requireHidden(window);
    const read = async () => nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
    const before = await read();
    if (before.state.readerConnected || before.state.boxReady)
        throw new Error("Extension verification requires disconnected physical sources");
    const original = before.values[cs2CoverEnabledSetting] === true;
    const originalDamage = before.values[cs2RemainingDamageEnabledSetting] === true;
    const originalHead = before.values[cs2HeadDamageEnabledSetting] === true;
    const originalBest = before.values[cs2BestDamageEnabledSetting] === true;
    const selector = '.esp-settings section[aria-label="掩体检测扩展"]';
    await waitDom(window, `!!document.querySelector(${JSON.stringify(selector)})?.querySelector('[role="switch"]')`, "Extension slot did not load its CS2 panel");
    const setEnabled = (enabled: boolean) => window.webContents.executeJavaScript(
        "window.mousebox.invoke('settings.set'," + JSON.stringify({ key: cs2CoverEnabledSetting, value: enabled }) + "," + JSON.stringify(context) + ")");
    try {
        await setEnabled(true);
        await waitDom(window, `document.querySelector('[data-extension-id="${cs2CoverExtensionId}"]')?.getAttribute('data-extension-phase')==='paused'`, "Hidden ESP extension did not pause");
        const state = await read();
        const extension = state.state.extensions?.find(item => item.id === cs2CoverExtensionId);
        if (extension?.phase !== "paused" || extension.queries !== 0 || state.state.cs2Map !== null)
            throw new Error("An unready hidden extension advertised analysis or retained a map");
        if (!(await readHelp(window,"启用掩体检测")).includes('静态碰撞场景'))
            throw new Error("Extension panel omitted its static scene coverage");
        await waitDom(window, "!!document.querySelector('.esp-settings section[aria-label=\"穿透余伤扩展\"]')", "Remaining damage extension panel did not load");
        await window.webContents.executeJavaScript("window.mousebox.invoke('settings.set'," +
            JSON.stringify({ key: cs2RemainingDamageEnabledSetting, value: true }) + "," + JSON.stringify(context) + ")");
        await waitDom(window, `document.querySelector('[data-extension-id="${cs2RemainingDamageExtensionId}"]')?.getAttribute('data-extension-phase')==='paused'`, "Hidden remaining damage analysis did not pause");
        const damage = (await read()).state.extensions?.find(item => item.id === cs2RemainingDamageExtensionId);
        if (damage?.queries !== 0 || damage.phase !== "paused")
            throw new Error("Hidden damage analysis performed work without a visible ESP surface");
        await waitDom(window, "!!document.querySelector('.esp-settings section[aria-label=\"头部伤害扩展\"]')", "Head damage extension panel did not load");
        if(!await window.webContents.executeJavaScript("!!(document.querySelector('section[aria-label=\"穿透余伤扩展\"]').compareDocumentPosition(document.querySelector('section[aria-label=\"头部伤害扩展\"]'))&Node.DOCUMENT_POSITION_FOLLOWING)"))
            throw new Error("Head damage switch must follow remaining damage");
        await window.webContents.executeJavaScript("window.mousebox.invoke('settings.set'," +
            JSON.stringify({ key: cs2HeadDamageEnabledSetting, value: true }) + "," + JSON.stringify(context) + ")");
        await waitDom(window, `document.querySelector('[data-extension-id="${cs2HeadDamageExtensionId}"]')?.getAttribute('data-extension-phase')==='paused'`, "Hidden head damage analysis did not pause");
        const head=(await read()).state.extensions?.find(item=>item.id===cs2HeadDamageExtensionId);
        if(head?.queries!==0||head.phase!=="paused") throw new Error("Hidden head damage performed geometry queries");
        await waitDom(window, "!!document.querySelector('.esp-settings section[aria-label=\"最高伤害瞄准点扩展\"]')", "Best damage panel did not load");
        await window.webContents.executeJavaScript("window.mousebox.invoke('settings.set'," +
            JSON.stringify({ key: cs2BestDamageEnabledSetting, value: true }) + "," + JSON.stringify(context) + ")");
        await waitDom(window, `document.querySelector('[data-extension-id="${cs2BestDamageExtensionId}"]')?.getAttribute('data-extension-phase')==='paused'`, "Hidden best damage analysis did not pause");
        const best=(await read()).state.extensions?.find(item=>item.id===cs2BestDamageExtensionId);
        if(best?.queries!==0||best.phase!=="paused") throw new Error("Hidden best damage search performed geometry queries");
    } finally {
        await setEnabled(original);
        await window.webContents.executeJavaScript("window.mousebox.invoke('settings.set'," +
            JSON.stringify({ key: cs2RemainingDamageEnabledSetting, value: originalDamage }) + "," + JSON.stringify(context) + ")");
        await window.webContents.executeJavaScript("window.mousebox.invoke('settings.set'," +
            JSON.stringify({ key: cs2HeadDamageEnabledSetting, value: originalHead }) + "," + JSON.stringify(context) + ")");
        await window.webContents.executeJavaScript("window.mousebox.invoke('settings.set'," +
            JSON.stringify({ key: cs2BestDamageEnabledSetting, value: originalBest }) + "," + JSON.stringify(context) + ")");
    }
    return { slot: true, remainingDamageSlot: true, headDamageSlot: true, bestDamageSlot: true, orderedHeadDamage: true, scopedSettings: true, hiddenPaused: true, staticScopeExplicit: true, geometryQueries: 0, physicalOutput: false };
}
