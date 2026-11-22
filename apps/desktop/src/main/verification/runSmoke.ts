import { writeFile } from "node:fs/promises";
import { resolve } from "node:path";
import {bootstrapSchema} from "@mousebox/shared";
import type { SmokeContext } from "./context";
import { verifyNavigation } from "./navigation";
import { verifyMouseDebug } from "./mouseDebug";
import { verifySettings } from "./settings";
import { verifyNativeLifecycle } from "./lifecycle";
import { verifyPageContext } from "./pageContext";
import { verifyWorkspaces } from "./workspaces";
import { verifySidebar } from "./sidebar";
import { requireHidden } from "./dom";
import { verifyTypography } from "./typography";
import {verifyEspTiming} from "./espTiming";
import {verifySourceData} from "./sourceData";
import {verifyPageTabs} from "./pageTabs";

/** Explicitly isolated hidden verification; physical actions are rejected at the native boundary. */
export async function runSmoke(context: SmokeContext): Promise<void> {
    const { window, container, report, native } = context;
    requireHidden(window);
    const originalRequest = native.request.bind(native);
    const blockedNativeActions: string[] = [];
    native.request = async (name, payload, timeout) => {
        if (["reader.connect", "box.connect", "calibration.start", "learning.start", "mouseReports.start"].includes(name) || name.endsWith(".calibration.start") ||
            name === "test.setArmed" && !!(payload && typeof payload === "object" && "armed" in payload && payload.armed)) {
            blockedNativeActions.push(name);
            throw new Error("Physical action is forbidden in hidden verification: " + name);
        }
        return originalRequest(name, payload, timeout);
    };
    try {
        const bootstrap = bootstrapSchema.parse(await container.invoke("bootstrap", {}));
        const typography = await verifyTypography(context);
        const nativeCapture = await native.request("capture.inspect") as {
            connected: boolean; processId: number; active: boolean; method: string; rawReportsThroughJs: boolean;
        };
        if (!nativeCapture.connected || nativeCapture.processId !== process.pid || nativeCapture.active ||
            nativeCapture.method !== "foreground-buffered-raw-input" || nativeCapture.rawReportsThroughJs)
            throw new Error("Foreground native capture endpoint failed its inactive process-identity check");
        const navigation = await verifyNavigation(context, bootstrap);
        const sidebar = await verifySidebar(context);
        const pageContext = await verifyPageContext(context);
        const settings = await verifySettings(context);
        const workspaces = await verifyWorkspaces(context,bootstrap.preferences);
        const espTiming=await verifyEspTiming(context);
        const sourceData=await verifySourceData(context);
        const pageTabs=await verifyPageTabs(context);
        const mouseDebugAudit = await verifyMouseDebug(context);
        const lifecycle = await verifyNativeLifecycle(context);
        if (blockedNativeActions.length) throw new Error("Unexpected physical actions were attempted: " + blockedNativeActions.join(","));
        await writeFile(resolve(report), JSON.stringify({ ok: true, bootstrap, ...navigation,
            typography, nativeCapture, sidebar, pageContext, workspaces, espTiming, sourceData, pageTabs, mouseDebugAudit, ...settings, ...lifecycle,
            blockedNativeActions, visible: window.isVisible(), focused: window.isFocused(), screenshotsTaken: pageTabs.previews.length>0||workspaces.screenshotsTaken||settings.screenshotsTaken,desktopScreenshotsTaken:false }, null, 2));
    } catch (error) {
        await writeFile(resolve(report), JSON.stringify({ ok: false, error: String(error), stack:error instanceof Error?error.stack:undefined, blockedNativeActions, visible: window.isVisible(), focused: window.isFocused(), screenshotsTaken: false }));
        process.exitCode = 1;
    } finally {
        native.request = originalRequest;
        window.close();
    }
}

