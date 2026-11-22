import type { WorkspaceContext } from "@mousebox/shared";
import type { SmokeContext } from "./context";
import { requireHidden, waitDom } from "./dom";

/** Synthetic display data only; no sampling, connection or input command. */
export async function verifyViewpoint({ window, container }: Pick<SmokeContext, "window" | "container">,
    context: WorkspaceContext, original: Record<string, unknown>) {
    requireHidden(window);
    const sample = { readOnly: true, capture: "9007199254740993", pawn: "101", bodyMatched: true,
        inputAngles: [-89, 45, 0], pawnInputAngles: [1, 2, 0], readerEyePosition: [103, 198, 92],
        bodyPosition: [100, 200, 32], rawViewOffset: [0, 0, 64], bodyRotation: [0, 90, 0], bodyScale: 1 };
    const publish = () => container.publish("state", { ...original, cs2Viewpoint: sample }, context);
    const timer = setInterval(publish, 50);
    try {
        publish();
        await waitDom(window, "document.querySelector('.viewpoint-diagnostics')?.textContent.includes('103.0000') && document.querySelector('.viewpoint-diagnostics')?.textContent.includes('-89.0000')", "Viewpoint dashboard did not show the observed coordinates and pitch");
        if (!await window.webContents.executeJavaScript(`(() => {
            const panel = document.querySelector('.viewpoint-diagnostics');
            return panel.querySelectorAll('thead th').length === 4 &&
                panel.textContent.includes('Reader 未提供') && !panel.querySelector('input,button:not(.help-trigger)');
        })()`)) throw new Error("Viewpoint display must remain read-only and distinguish unavailable native data");
    } finally {
        clearInterval(timer);
        container.publish("state", original, context);
    }
    return { coordinateColumns: true, separateAngleSources: true, readOnly: true, physicalSampling: false };
}
