import { nativeSnapshotSchema } from "@mousebox/shared";
import type { SmokeContext } from "./context";
import { requireHidden } from "./dom";

export async function verifyNativeLifecycle({ window, native, container }: SmokeContext) {
    requireHidden(window);
    const snapshot = nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
    const context = snapshot.workspace?.active ?? undefined;
    if (!context) throw new Error("Lifecycle verification needs an active native workspace");
    const nativeSurface = await container.invoke("surface.update", {
        kind: "esp", x: 260, y: 80, width: 600, height: 480, scale: 1, visible: true,
    }, context) as { childHandle?: string };
    if (!nativeSurface.childHandle || nativeSurface.childHandle === "0")
        throw new Error("Cross-process native child was not created");
    await container.invoke("surface.update", { kind: "esp", x: 260, y: 80, width: 600, height: 480, scale: 1, visible: false }, context);
    window.webContents.forcefullyCrashRenderer();
    await new Promise(resolve => setTimeout(resolve, 300));
    const revoked = nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
    if (revoked.state.sessionEligible) throw new Error("Renderer crash did not revoke the native session");
    return { nativeSurface, rendererCrashRevoked: true, foregroundInteraction: false, screenshotsTaken: false };
}

