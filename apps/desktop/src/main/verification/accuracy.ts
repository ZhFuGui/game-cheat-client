import { writeFile } from "node:fs/promises";
import { dirname, join } from "node:path";
import { cs2AccuracySchema, type Cs2Accuracy, type WorkspaceContext } from "@mousebox/shared";
import type { SmokeContext } from "./context";
import { requireHidden, waitDom } from "./dom";

/** Synthetic presentation proof; never connects a Reader or grants output. */
export async function verifyAccuracy({ window, container, args, report }: Pick<SmokeContext, "window" | "container" | "args" | "report">,
    context: WorkspaceContext, original: Record<string, unknown>) {
    requireHidden(window);
    let sample: Cs2Accuracy | null = cs2AccuracySchema.parse({
        readOnly: true, capture: "9007199254740993", weapon: "32773", owner: "32770",
        modelId: "cs2.weapon_accuracy.14193.v1", branch: "normal",
        inaccuracy: { status: "valid", value: 0, reason: "" }, spread: { status: "valid", value: .00125, reason: "" },
        components: { penalty: 0, movement: 0, airborne: 0, turning: 0, lateral: 0 }, readIntervalMs: .025,
    });
    const publish = () => container.publish("state", { ...original, cs2Accuracy: sample }, context);
    const timer = setInterval(publish, 50), previews: string[] = [];
    try {
        publish();
        await waitDom(window, "document.querySelector('.accuracy-diagnostics')?.textContent.includes('0.0012500')", "Accuracy dashboard did not show separate spread");
        const read = (label: string) => "[...document.querySelectorAll('.accuracy-diagnostics dt')].find(e=>e.textContent===" + JSON.stringify(label) + ")?.nextElementSibling?.textContent";
        if (await window.webContents.executeJavaScript(read("完整 Inaccuracy（无量纲）")) !== "0.0000000")
            throw new Error("Accuracy dashboard lost the valid zero");
        if (args.includes("--preview-accuracy")) {
            const [width, height] = window.getContentSize();
            // Prime this hidden backing surface before capturing the new tab.
            await window.webContents.capturePage({ x: 0, y: 0, width, height }, { stayHidden: true, stayAwake: true });
            await window.webContents.executeJavaScript("new Promise(resolve=>setTimeout(resolve,100))");
            const capture = await window.webContents.capturePage({ x: 0, y: 0, width, height }, { stayHidden: true, stayAwake: true });
            if (capture.isEmpty()) throw new Error("Hidden accuracy preview was empty");
            const file = join(dirname(report), "accuracy.png"); await writeFile(file, capture.toPNG()); previews.push(file);
        }
        sample = { ...sample, branch: "unavailable", inaccuracy: { status: "unavailable", value: null, reason: "速度缓存等待原生重算" }, components: null };
        publish();
        await waitDom(window, read("完整 Inaccuracy（无量纲）") + "==='—' && " + read("独立 Spread（无量纲）") + "==='0.0012500'", "Accuracy failure did not preserve independent spread");
        sample = null; publish();
        await waitDom(window, read("独立 Spread（无量纲）") + "==='—'", "Invalidation kept an old accuracy result");
    } finally {
        clearInterval(timer); container.publish("state", original, context);
    }
    return { readOnly: true, validZero: true, independentResults: true, invalidationCleared: true, physicalSampling: false, previews };
}
