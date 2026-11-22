import { nativeSnapshotSchema, type WorkspaceContext } from "@mousebox/shared";
import type { SmokeContext } from "./context";
import { requireHidden, waitDom } from "./dom";

/** Display settings exercise real IPC in isolated storage, with no source or output. */
export async function verifyHitboxes({ window, native }: Pick<SmokeContext, "window" | "native">, context: WorkspaceContext) {
    requireHidden(window);
    const read = async () => nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
    const before = await read();
    if (before.state.readerConnected || before.state.boxReady)
        throw new Error("Hitbox verification requires disconnected physical sources");
    const original = { cs2ShowHealth: before.values.cs2ShowHealth, cs2HitboxDisplay: before.values.cs2HitboxDisplay };
    const set = (key: keyof typeof original, value: unknown) => window.webContents.executeJavaScript(
        "window.mousebox.invoke('settings.set'," + JSON.stringify({ key, value }) + "," + JSON.stringify(context) + ")");
    const hpSwitch = "[...document.querySelectorAll('.esp-settings .toggle-row')].find(e=>e.textContent==='血量')?.querySelector('[role=\"switch\"]')";
    const requests:unknown[]=[];
    const request=native.request.bind(native);
    native.request=async(name,payload,timeout)=>{
        if(name==="settings.set"&&payload&&typeof payload==="object"&&"key" in payload&&payload.key==="cs2ShowHealth")requests.push(payload);
        return request(name,payload,timeout);
    };
    try {
        await waitDom(window, `!!(${hpSwitch}) && !!document.querySelector('[aria-label="Hitbox 命中体"]')`, "CS2 Hitbox and health controls did not render");
        await set("cs2ShowHealth", true);
        await waitDom(window, `(${hpSwitch})?.getAttribute('aria-checked')==='true'`, "Health control did not reflect native state");
        await window.webContents.executeJavaScript(`(${hpSwitch}).click()`);
        try{
            await waitDom(window, `(${hpSwitch})?.getAttribute('aria-checked')==='false'`, "Health display did not switch off");
        }catch(error){
            const facts=await window.webContents.executeJavaScript(`({button:(${hpSwitch})?.outerHTML,toasts:[...document.querySelectorAll('[data-sonner-toast]')].map(e=>e.textContent)})`);
            throw new Error(String(error)+" / "+JSON.stringify({requests,facts,native:(await read()).values.cs2ShowHealth}));
        }
        await set("cs2HitboxDisplay", "wireframe");
        await waitDom(window, "document.querySelector('[aria-label=\"Hitbox 命中体\"]')?.textContent.includes('线框命中体')", "Hitbox mode did not reflect the saved choice");
        const state = await read();
        if (state.values.cs2ShowHealth !== false || state.values.cs2HitboxDisplay !== "wireframe" ||
            state.state.cs2Hitboxes?.phase !== "waiting-data" || state.state.canAim)
            throw new Error("Display settings fabricated a pose or changed output qualification");
        await window.webContents.executeJavaScript("[...document.querySelectorAll('.esp-panel-footer button')].find(b=>b.textContent==='恢复默认').click()");
        await waitDom(window, `(${hpSwitch})?.getAttribute('aria-checked')==='true' && document.querySelector('[aria-label="Hitbox 命中体"]')?.textContent.includes('关闭')`, "CS2 display defaults were not restored together");
        const reset = await read();
        if (reset.values.cs2ShowHealth !== true || reset.values.cs2HitboxDisplay !== "off")
            throw new Error("Reset did not persist CS2 display defaults");
    } finally {
        native.request=request;
        for (const [key, value] of Object.entries(original))
            await set(key as keyof typeof original, value);
    }
    return { healthSwitch: true, wireframeSetting: true, resetDefaults: true, missingPoseExplicit: true, physicalOutput: false };
}
