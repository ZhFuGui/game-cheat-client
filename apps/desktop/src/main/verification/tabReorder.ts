import { join } from "node:path";
import { bootstrapSchema } from "@mousebox/shared";
import { UiPreferenceStore } from "../infrastructure/storage/UiPreferences";
import type { SmokeContext } from "./context";
import { navigate, requireHidden, waitDom } from "./dom";

/** Synthetic pointer events stay inside the isolated renderer; no desktop mouse input. */
export async function verifyTabReorder({ window, container, args }: SmokeContext) {
  requireHidden(window);
  await navigate(window, "/", ".reader-connection-panel");
  const original = bootstrapSchema.parse(await container.invoke("bootstrap", {})).preferences.pageTabOrder?.reader ?? ["connection", "dashboard"];
  try {
    const result = await window.webContents.executeJavaScript(`(()=>{
      const tabs=[...document.querySelectorAll('.reader-page [role=tab]')],first=tabs[0],last=tabs[tabs.length-1];
      const selected=document.querySelector('.reader-page [aria-selected=true]').id;
      const panel=document.querySelector('.reader-page [role=tabpanel]');panel.dataset.reorderProbe='retained';
      const a=first.getBoundingClientRect(),b=last.getBoundingClientRect();
      const x=a.left+a.width/2,y=a.top+a.height/2,end=b.left+b.width/2+2;
      first.dispatchEvent(new PointerEvent('pointerdown',{bubbles:true,cancelable:true,pointerId:31,button:0,buttons:1,clientX:x,clientY:y}));
      window.dispatchEvent(new PointerEvent('pointermove',{bubbles:true,cancelable:true,pointerId:31,buttons:1,clientX:end,clientY:y}));
      const preview=document.querySelector('.reader-page .page-tabs').dataset.reordering==='true';
      window.dispatchEvent(new PointerEvent('pointerup',{bubbles:true,pointerId:31,button:0,buttons:0,clientX:end,clientY:y}));
      first.dispatchEvent(new MouseEvent('click',{bubbles:true,cancelable:true,detail:1}));
      return {before:tabs.map(tab=>tab.dataset.tabId),selected,preview};
    })()`) as { before: string[]; selected: string; preview: boolean };
    const expected = [...result.before.slice(1), result.before[0]];
    await waitDom(window, `JSON.stringify([...document.querySelectorAll('.reader-page [role=tab]')].map(tab=>tab.dataset.tabId))===${JSON.stringify(JSON.stringify(expected))}`, "Tab drop did not reorder");
    const stable = await window.webContents.executeJavaScript(`(()=>{
      const page=document.querySelector('.reader-page');return page.querySelector('[aria-selected=true]').id===${JSON.stringify(result.selected)}
        &&page.querySelector('[role=tabpanel]').dataset.reorderProbe==='retained'&&!page.querySelector('[data-reordering]');
    })()`) as boolean;
    const saved = bootstrapSchema.parse(await container.invoke("bootstrap", {})).preferences.pageTabOrder?.reader;
    const directory = args.find(arg => arg.startsWith("--data-dir="))?.slice("--data-dir=".length);
    if (!directory) throw new Error("Tab reorder verification needs isolated preferences");
    const reopened = new UiPreferenceStore(join(directory, "web-ui.json")).get().pageTabOrder?.reader;
    if (!result.preview || !stable || JSON.stringify(saved) !== JSON.stringify(expected) || JSON.stringify(reopened) !== JSON.stringify(expected))
      throw new Error("Tab reorder changed content or did not persist: " + JSON.stringify({ result, stable, saved, reopened }));
    await navigate(window, "/mouse-debug", ".mouse-debug-page");
    await navigate(window, "/", ".reader-connection-panel");
    const restored = await window.webContents.executeJavaScript("[...document.querySelectorAll('.reader-page [role=tab]')].map(tab=>tab.dataset.tabId)");
    if (JSON.stringify(restored) !== JSON.stringify(expected)) throw new Error("Tab order did not survive page remount");
    return { syntheticPointer: true, reordered: true, selectedContentRetained: stable, saved: true, reopened: true, pageRemount: true, physicalInput: false };
  } finally {
    await container.invoke("preferences.set", { pageTabOrder: { reader: original } });
    requireHidden(window);
  }
}
