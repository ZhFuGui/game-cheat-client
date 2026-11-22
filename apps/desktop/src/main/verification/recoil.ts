import { nativeSnapshotSchema, type WorkspaceContext } from "@mousebox/shared";
import type { SmokeContext } from "./context";
import { navigate, requireHidden, waitDom } from "./dom";

export async function verifyRecoil({ window, native }: Pick<SmokeContext, "window" | "native">, context: WorkspaceContext) {
    requireHidden(window);
    const read = async () => nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
    const initial = await read();
    if (initial.state.readerConnected || initial.state.boxReady || initial.state.aimArmed)
        throw new Error("Recoil verification requires a disconnected application");
    await navigate(window, "/recoil", ".recoil-page");
    await waitDom(window, "document.querySelector('.recoil-page [role=tabpanel]')?.childElementCount===0 && [...document.querySelectorAll('.recoil-page [role=tab]')].map(tab=>tab.textContent).join('')==='No Recoil'", "The retired recoil tab is still exposed");
    await navigate(window, "/aim?section=aimbot", ".aimbot-page");
    await waitDom(window, "document.querySelector('.aimbot-page')?.textContent.includes('补偿发射后坐偏移')", "Aimbot's recoil switch did not render");
    const recoilEnabled = initial.values.cs2RecoilCompensation;
    const setEnabled = (value: boolean) => window.webContents.executeJavaScript(
        "window.mousebox.invoke('settings.set'," + JSON.stringify({ key: "cs2RecoilCompensation", value }) + "," + JSON.stringify(context) + ")");
    try {
        if (typeof recoilEnabled !== "boolean") throw new Error("Missing shared recoil preference");
        await setEnabled(!recoilEnabled);
        const updated = await read();
        if (updated.values.cs2RecoilCompensation === recoilEnabled || updated.values.trigger !== initial.values.trigger ||
            updated.values.aimEnabled !== initial.values.aimEnabled || updated.state.cs2Controller?.recoilOnly ||
            "cs2RcsEnabled" in updated.values)
            throw new Error("Changing the recoil setting affected the wrong activation or state");
    } finally {
        if (typeof recoilEnabled === "boolean") await setEnabled(recoilEnabled);
    }
    return { integrated: true, retiredTab: true, emptyNoRecoil: true, sideButtonPreserved: true, physicalOutput: false };
}
