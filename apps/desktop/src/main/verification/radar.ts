import { writeFile } from "node:fs/promises";
import { dirname, join } from "node:path";
import { cs2RadarReplySchema, type Cs2RadarFrame, type WorkspaceContext } from "@mousebox/shared";
import type { SmokeContext } from "./context";
import { navigate, requireHidden, waitDom } from "./dom";

function withLocalHeight(frame: Cs2RadarFrame, revision: string, height: number | null): Cs2RadarFrame {
  return { ...frame, revision, players: frame.players.map(player => player.local && player.position ?
    { ...player, position: height === null ? null : [player.position[0], player.position[1], height] } : player) };
}

export async function verifyRadar({ window, native, container, args, report }:
  Pick<SmokeContext, "window" | "native" | "container" | "args" | "report">, context: WorkspaceContext) {
  requireHidden(window);
  const disconnected = cs2RadarReplySchema.parse(await container.invoke("game.cs2.radar.read", {}, context));
  if (disconnected.frame || disconnected.unchanged) throw new Error("An isolated Radar exposed a current frame");
  const original = native.request.bind(native);
  let reads = 0;
  const nuke: Cs2RadarFrame = {
    revision: "1", scope: "1:1:100", map: {
      name: "de_nuke", resourcePath: "maps/de_nuke.vpk", status: 1, available: true, generation: "1",
    }, players: [
      { controller: 101, name: "Nova", local: true, team: 3, alive: true, position: [80, -1665, 0], yaw: 45,
        health: 86, maxHealth: 100, armor: 100, weapon: { name: "weapon_m4a1", clip: 25, reserve: 75 } },
      { controller: 102, name: "Iris", local: false, team: 3, alive: true, position: [635, 395, 0], yaw: 115,
        health: 62, maxHealth: 100, armor: 48, weapon: { name: "weapon_awp", clip: 4, reserve: 20 } },
      { controller: 201, name: "Atlas", local: false, team: 2, alive: true, position: [80, -1665, -620], yaw: -90,
        health: 100, maxHealth: 100, armor: 75, weapon: { name: "weapon_ak47", clip: 29, reserve: 90 } },
      { controller: 202, name: "Echo", local: false, team: 2, alive: false, position: null, yaw: null,
        health: 0, maxHealth: 100, armor: 0, weapon: null },
    ],
  };
  let frame: Cs2RadarFrame | null = nuke;
  const standalone = (await native.request("runtime.snapshot")) as { state?: { readerConnected?: boolean } };
  if (standalone.state?.readerConnected) throw new Error("Radar smoke requires a disconnected Reader");
  native.request = async (name, payload, timeout) => {
    if (name !== "game.cs2.radar.read") return original(name, payload, timeout);
    ++reads;
    if (frame && payload && typeof payload === "object" && "after" in payload && payload.after === frame.revision)
      return { frame: null, unchanged: true };
    return { frame, unchanged: false };
  };
  const previews: string[] = [];
  try {
    await window.webContents.executeJavaScript("void Object.defineProperty(document,'hidden',{configurable:true,value:false})");
    await navigate(window, "/visuals", ".esp-page");
    await window.webContents.executeJavaScript("document.getElementById('visuals-tab-radar').click()");
    const canvas = "document.querySelector('.radar-canvas[data-map-ready=true]')";
    const layer = "document.querySelector('.radar-map')?.dataset.mapLayer";
    const auto = "document.querySelector('[aria-label=\"自动切层\"]')";
    const waitLevel = (value: "main" | "lower", description: string) =>
      waitDom(window, `!!${canvas} && ${layer} === '${value}'`, description);
    await waitDom(window, `${canvas}?.dataset.players === '3' && document.querySelector('.radar-roster-t')?.textContent.includes('0 HP') &&
      document.querySelector('.radar-roster-ct')?.textContent.includes('25 / 75') && !document.querySelector('[data-native-surface=esp]')`,
    "CS2 Radar did not load the calibrated map, ready markers and team rosters");
    await waitDom(window, `${layer} === 'main' && ${auto}?.getAttribute('aria-pressed') === 'true'`,
      "Nuke did not start in automatic floor mode");
    const layout = await window.webContents.executeJavaScript(`(() => {
      const page = document.querySelector('.radar-page'), grid = document.querySelector('.radar-layout');
      const old = page.style.width;
      try {
        page.style.width = '699px';
        const below = getComputedStyle(grid).gridTemplateAreas;
        page.style.width = '700px';
        return {below, at: getComputedStyle(grid).gridTemplateAreas};
      } finally { page.style.width = old; }
    })()`) as { below: string; at: string };
    if (layout.below !== '"map map" "ct t"' || layout.at !== '"ct map t"')
      throw new Error(`Radar layout threshold did not switch at 700 CSS px: ${JSON.stringify(layout)}`);
    if (args.includes("--preview-radar")) {
      await window.webContents.capturePage(undefined, { stayHidden: true, stayAwake: true });
      await window.webContents.executeJavaScript("new Promise(resolve=>setTimeout(resolve,100))");
      const picture = await window.webContents.capturePage(undefined, { stayHidden: true, stayAwake: true });
      if (picture.isEmpty()) throw new Error("Radar preview was empty");
      const file = join(dirname(report), "cs2-radar.png");
      await writeFile(file, picture.toPNG());
      previews.push(file);
    }
    await window.webContents.executeJavaScript("[...document.querySelectorAll('.radar-controls button')].find(b=>b.textContent==='跟随本地').click()");
    await waitDom(window, `${canvas}?.dataset.following === 'true'`, "The local camera did not enter follow mode");
    await window.webContents.executeJavaScript("document.querySelector('.radar-zoom [aria-label=放大雷达]').click()");
    await waitDom(window, "document.querySelector('.radar-zoom').textContent.includes('125%')", "Radar zoom did not update");
    frame = withLocalHeight(nuke, "2", -496);
    await waitLevel("lower", "Nuke did not switch to the lower overview automatically");
    await window.webContents.executeJavaScript("[...document.querySelectorAll('.radar-controls button')].find(b=>b.textContent==='主层').click()");
    await waitLevel("main", "Manual override did not show the main level");
    await window.webContents.executeJavaScript(`${auto}.click()`);
    await waitLevel("lower", "Returning to auto did not restore the local player's level");
    frame = withLocalHeight(nuke, "3", null);
    await waitDom(window, `${canvas}?.dataset.players === '2' && ${layer} === 'lower'`,
      "A missing local position discarded the last known floor");
    const vertigo: Cs2RadarFrame = {
      ...nuke, map: { name: "de_vertigo", resourcePath: "maps/de_vertigo.vpk", generation: "2", available: true, status: 1 },
    };
    frame = withLocalHeight(vertigo, "4", 11699);
    await waitDom(window, `!!${canvas} && document.querySelector('.radar-map').dataset.map === 'de_vertigo' && ${layer} === 'lower'`,
      "Vertigo did not pick its local lower floor");
    frame = withLocalHeight(vertigo, "5", 11700);
    await waitLevel("main", "Vertigo did not pick the main floor at its boundary");
    frame = null;
    await waitDom(window, "!document.querySelector('.radar-canvas') && !document.querySelector('.radar-roster li') && document.querySelector('.radar-map-status').textContent.includes('等待 CS2 Reader 数据')",
      "Radar kept an invalidated map or roster");
    await navigate(window, "/visuals", ".esp-page");
    const closed = reads;
    await window.webContents.executeJavaScript("new Promise(resolve=>setTimeout(resolve,230))");
    if (reads !== closed) throw new Error("Radar kept polling after its tab was closed");
    requireHidden(window);
    return { map: true, roster: true, follow: true, zoom: true, lowerLayer: true, automaticFloors: true,
      manualOverride: true, missingHeightFallback: true, layout, clearedFrame: true, stoppedOnLeave: true, previews };
  } finally {
    native.request = original;
    await window.webContents.executeJavaScript("delete document.hidden");
  }
}
