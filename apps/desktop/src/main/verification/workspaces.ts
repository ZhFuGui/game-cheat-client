import { nativeSnapshotSchema, workspaceCatalogSchema, type CommandName, type WorkspaceContext, type UiPreferences } from "@mousebox/shared";
import type { SmokeContext } from "./context";
import { navigate, requireHidden, waitDom } from "./dom";
import { verifyRecoil } from "./recoil";
import { verifyTriggerbot } from "./triggerbot";
import { verifyRage } from "./rage";
import { verifyExtensions } from "./extensions";
import { verifyHitboxes } from "./hitboxes";
import { verifyViewpoint } from "./viewpoint";
import { verifyAccuracy } from "./accuracy";
import {verifyEspOutput} from "./espOutput";
import {verifyRadar} from "./radar";
import {verifyAimMovement} from "./aimMovement";
import {verifyAimbotLayout} from "./aimbotLayout";
import {verifyAimGeneral} from "./aimGeneral";
import {verifyFixedTabs} from "./fixedTabs";
import {verifyAimActivation} from "./aimActivation";
import {verifyAimLayout} from "./aimLayout";

/** Exercises the real game dropdown in an isolated, hidden renderer. */
export async function verifyWorkspaces({ window, native, container, args, report }: SmokeContext,preferences:UiPreferences) {
    requireHidden(window);
    const initial = nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
    const original = initial.workspace?.active;
    if (!original || !initial.application) throw new Error("Game verification needs a native catalog");
    const rendererInvoke = <T>(name: CommandName, args: unknown, context?: WorkspaceContext): Promise<T> =>
        window.webContents.executeJavaScript("window.mousebox.invoke("+JSON.stringify(name)+","+JSON.stringify(args)+","+(JSON.stringify(context)??"undefined")+")") as Promise<T>;
    const readCatalog = async () => workspaceCatalogSchema.parse(await native.request("workspace.read"));
    const waitActive = async (id: string, formerEpoch?: string) => {
        for (let n = 0; n < 150; n++) {
            const catalog = await readCatalog();
            if (catalog.active?.workspaceId === id && catalog.active.epoch !== formerEpoch) {
                await waitDom(window, "document.querySelector('.workspace-outlet')?.dataset.workspaceEpoch==="+JSON.stringify(catalog.active.epoch), "Game selector did not settle");
                // The native commit can precede the desktop snapshot refresh.
                try { await rendererInvoke("algorithm.read", {}, catalog.active); }
                catch (error) {
                    if (String(error).includes("Workspace switch is in progress")) {
                        await new Promise(resolve => setTimeout(resolve, 20));
                        continue;
                    }
                    throw error;
                }
                return catalog;
            }
            await new Promise(resolve => setTimeout(resolve, 50));
        }
        throw new Error("Game did not become active: " + id);
    };
    const chooseGame = async (name: string) => {
        const popup = "document.getElementById(document.querySelector('.game-selector [role=\"combobox\"]').getAttribute('aria-controls'))";
        await waitDom(window, "document.querySelector('.game-selector [role=\"combobox\"]')?.disabled===false", "Game selector is unavailable");
        await window.webContents.executeJavaScript("document.querySelector('.game-selector [role=\"combobox\"]').click()");
        await waitDom(window, "("+popup+")?.querySelector('[role=\"option\"]')", "Game option missing");
        await waitDom(window, "!!document.querySelector('.select-popup-menu')", "Compact game menu did not open");
        const options = await window.webContents.executeJavaScript("[...("+popup+").querySelectorAll('[role=\"option\"]')].map(option=>option.textContent)") as string[];
        if (options.join("|") !== "PUBG|Counter-Strike 2") throw new Error("The selector must only show integrated game names: "+JSON.stringify(options));
        // Base UI requires pointer provenance for an unhighlighted item. This
        // DOM-only touch press stays inside the hidden renderer, with no OS input.
        await window.webContents.executeJavaScript("(()=>{const option=[...("+popup+").querySelectorAll('[role=\"option\"]')].find(option=>option.textContent==="+JSON.stringify(name)+");option.dispatchEvent(new PointerEvent('pointerdown',{bubbles:true,pointerType:'touch'}));option.click()})()");
    };
    const directory = await readCatalog();
    const cs2Id = directory.workspaces.find(entry => entry.gameId === "cs2")?.id;
    if (!cs2Id || directory.workspaces.length !== 2 || directory.games.find(game => game.id === "gta5")?.status !== "unavailable")
        throw new Error("There must be one configuration for each integrated game");
    if (await window.webContents.executeJavaScript("!!document.querySelector('a[href=\"/workspaces\"]') || !!document.querySelector('.workspace-create,.workspace-rename,.workspaces-page')"))
        throw new Error("Removed workspace management is still exposed");
    let rejectedMissing = false;
    let results: Record<string, unknown> | undefined;
    try {
        let pubgAim,pubgGeneral;
        if(original.gameId==="pubg") {
            await navigate(window, "/aim", ".aimbot-page");
            pubgAim=await verifyAimbotLayout(window,args,report,"pubg",native);
            pubgGeneral=await verifyAimGeneral({window,native,args,report},"pubg");
        }
        await navigate(window, "/", ".reader-page");
        await chooseGame("Counter-Strike 2");
        const switched = await waitActive(cs2Id, original.epoch);
        const snapshot = nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
        if (snapshot.application!.dpi !== initial.application.dpi || snapshot.application!.renderAdapter !== initial.application.renderAdapter)
            throw new Error("Switching replaced shared hardware settings");
        if ("gravity" in snapshot.values || typeof snapshot.values.cs2MaxAgeMs !== "number" || snapshot.state.aimArmed !== false)
            throw new Error("CS2 inherited PUBG configuration or output state");
        await waitDom(window, "document.querySelector('.reader-connection-panel input')?.value==='127.0.0.1' && !!document.querySelector('nav a[href=\"/aim\"]')", "CS2 capability navigation did not settle");
        await navigate(window, "/aim", ".aimbot-page");
        await waitDom(window, "document.querySelectorAll('.aim-page [role=tablist]').length===1 && ['general','aimbot','triggerbot','rage'].every(id=>document.querySelector('.aim-page [data-aim-section='+id+']')) && !['/aimbot','/triggerbot','/rage'].some(path=>document.querySelector('nav a[href=\"'+path+'\"]'))", "Aim did not group the children under one navigation entry");
        await waitDom(window, "document.querySelector('.aimbot-page')?.textContent.includes('CS2 跟随设置') && !['运行控制','链路就绪后自动启用','CS2 设备计数校准','立即停止所有修正','完整人物句柄','等待反映的已发报文'].some(text=>document.querySelector('.aimbot-page')?.textContent.includes(text)) && document.querySelector('.page-context-bar')?.dataset.contextPage==='aimbot'", "CS2 Aimbot runtime status did not move exclusively to the page context bar");
        const cs2Aim=await verifyAimbotLayout(window,args,report,"cs2",native);
        const fixedTabs=await verifyFixedTabs({window,container,args,report});
        const aimGeneral=await verifyAimGeneral({window,native,args,report},"cs2");
        const movement=await verifyAimMovement({window,native,args,report},switched.active!);
        const recoil = await verifyRecoil({ window, native }, switched.active!);
        const triggerbot = await verifyTriggerbot({window,native,args,report},switched.active!);
        const rage = await verifyRage({window,native,args,report},switched.active!);
        const aimActivation=await verifyAimActivation({window,native,args,report},switched.active!);
        const aimLayoutDensity=await verifyAimLayout({window,args,report});
        await navigate(window, "/?section=dashboard", ".data-dashboard-panel");
        await waitDom(window, "document.querySelector('.data-dashboard-panel')?.textContent.includes('CS2 当前数据') && !document.querySelector('.data-dashboard-panel input')", "CS2 telemetry did not move to its dashboard");
        const viewpoint = await verifyViewpoint({ window, container }, switched.active!, snapshot.state);
        const accuracy = await verifyAccuracy({ window, container, args, report }, switched.active!, snapshot.state);
        await navigate(window, "/game-settings", '.settings-navigation button[data-section="cs2-source"]');
        await waitDom(window, "[...document.querySelectorAll('.settings-navigation button')].map(e=>e.dataset.section).join('|')==='cs2-source|aimbot-algorithm|aimbot-mouse'", "CS2 settings did not show its own data/algorithm/mouse panes");
        await navigate(window, "/visuals", ".esp-page");
        await waitDom(window, "document.querySelector('.esp-panel-heading h2')?.textContent==='CS2 ESP'", "CS2 projection settings did not render");
        const extensions = await verifyExtensions({ window, native }, switched.active!);
        const espOutput=await verifyEspOutput({window,native,container},preferences);
        const hitboxes = await verifyHitboxes({ window, native }, switched.active!);
        const radar = await verifyRadar({window,native,container,args,report}, switched.active!);

        // Display-only facts: no device or Reader connection is opened.
        const deviceIndicator = { phase: "ready" as const, message: "SYNTHETIC · device heartbeat" };
        container.publish("workspace", { ...switched, active: null });
        container.publish("devices", { boxReady: true, connections: { serial: deviceIndicator, box: deviceIndicator } });
        await waitDom(window, "document.querySelector('[data-link=\"box\"]')?.getAttribute('data-phase')==='ready'", "Global device facts froze without an active game");
        container.publish("state", { target: "STALE-GAME-SMOKE", boxReady: false }, original);
        await waitDom(window, "document.querySelector('[data-link=\"box\"]')?.getAttribute('data-phase')==='ready'", "Old game state overwrote global devices");
        container.publish("workspace", switched);
        if (snapshot.devices) container.publish("devices", snapshot.devices);
        for (const topic of ["values", "state", "espValues", "learning", "mouseReports"] as const)
            container.publish(topic, snapshot[topic], switched.active ?? undefined);
        try { await rendererInvoke("workspace.switch", { id: "missing-smoke-game" }); }
        catch { rejectedMissing = true; }
        if (!rejectedMissing) throw new Error("Missing game configuration was silently accepted");
        const failed = nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
        if (failed.state.sessionEligible !== false)
            throw new Error("Failed switch silently restored output eligibility");
        await chooseGame("Counter-Strike 2");
        const recovered = await waitActive(cs2Id, switched.active?.epoch);
        results = { original, switched: switched.active, recovered: recovered.active,
            singleConfigurationPerGame: true, directGameSelector: true, managementRemoved: true,
            pubgAim,pubgGeneral,
            cs2: { workspaceId: cs2Id, sourceAndEsp: true, aimAvailable: true, calibrationPanelRemoved: true, aimLayout:cs2Aim, aimLayoutDensity, aimGeneral, fixedTabs, movement, recoil, triggerbot, rage, aimActivation, extensions, espOutput, hitboxes, radar, viewpoint, accuracy, outputDisabled: true, independentSettings: true, readerConnected: false },
            globalSettingsPreserved: true, globalDevicesWithoutWorkspace: true, oldStateRejected: true,
            failedSwitchRevoked: true, selectorRetryRecovered: true,
            physicalHardwareTouched: false, foregroundInteraction: false, screenshotsTaken: !!pubgAim?.previews.length||cs2Aim.previews.length>0||aimLayoutDensity.previews.length>0||movement.previews.length>0||triggerbot.previews.length>0||rage.previews.length>0||accuracy.previews.length>0||radar.previews.length>0 };
    } finally {
        await rendererInvoke("workspace.switch", { id: original.workspaceId });
        const restored = nativeSnapshotSchema.parse(await native.request("runtime.snapshot"));
        if (restored.workspace) container.publish("workspace", restored.workspace);
        if (restored.application) container.publish("application", restored.application);
        if (restored.devices) container.publish("devices", restored.devices);
        const context = restored.workspace?.active ?? undefined;
        for (const topic of ["values", "state", "espValues", "learning", "mouseReports"] as const)
            container.publish(topic, restored[topic], context);
    }
    return results;
}

