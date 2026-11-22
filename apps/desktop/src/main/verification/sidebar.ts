import { SIDEBAR_DEFAULT_WIDTH, SIDEBAR_MAX_WIDTH, SIDEBAR_MIN_WIDTH, SIDEBAR_COLLAPSED_WIDTH, SIDEBAR_TOGGLE_DRAG_DISTANCE, type Bootstrap } from "@mousebox/shared";
import type { SmokeContext } from "./context";
import { navigate, requireHidden, waitDom } from "./dom";

/** Synthetic DOM pointer events in an isolated hidden renderer; no OS input. */
export async function verifySidebar({ window, container }: SmokeContext) {
    requireHidden(window);
    const initial = await container.invoke("bootstrap", {}) as Bootstrap;
    const original = initial.preferences.sidebarWidth ?? SIDEBAR_DEFAULT_WIDTH;
    const originallyCollapsed = initial.preferences.sidebarCollapsed ?? false;
    const [originalWidth, originalHeight] = window.getSize();
    const set = async (width: number, collapsed = false) => {
        await container.invoke("preferences.set", { sidebarWidth: width, sidebarCollapsed: collapsed });
        await waitDom(window, `Math.abs(document.querySelector('.sidebar').getBoundingClientRect().width-${collapsed ? SIDEBAR_COLLAPSED_WIDTH : width})<1`, "Sidebar did not restore its persisted width");
    };
    await navigate(window, "/visuals", ".esp-page");
    try {
        await set(SIDEBAR_DEFAULT_WIDTH);
        const target = SIDEBAR_MAX_WIDTH;
        const result = await window.webContents.executeJavaScript(`(async()=>{
          const rail=document.querySelector('.sidebar-resize-handle');
          const sidebar=document.querySelector('.sidebar');
          const start=sidebar.getBoundingClientRect().right;
          const before=document.querySelector('.esp-page').getBoundingClientRect();
          rail.dispatchEvent(new PointerEvent('pointerdown',{bubbles:true,button:0,buttons:1,pointerId:41,clientX:start}));
          window.dispatchEvent(new PointerEvent('pointermove',{buttons:1,pointerId:41,clientX:start+70}));
          await new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve)));
          const draft=sidebar.getBoundingClientRect().width;
          const active=document.documentElement.classList.contains('is-sidebar-resizing');
          window.dispatchEvent(new PointerEvent('pointerup',{pointerId:41,clientX:start+70}));
          await new Promise(resolve=>requestAnimationFrame(resolve));
          const after=document.querySelector('.esp-page').getBoundingClientRect();
          const boot=await window.mousebox.invoke('bootstrap',{});
          return {draft,active,saved:boot.preferences.sidebarWidth,ended:!document.documentElement.classList.contains('is-sidebar-resizing'),
            sidebar:sidebar.getBoundingClientRect().width,titlebarLeft:document.querySelector('.titlebar').getBoundingClientRect().left,
            workspaceLeft:document.querySelector('.workspace').getBoundingClientRect().left,contentDelta:after.left-before.left};
        })()`);
        if (!result.active || !result.ended || result.saved !== target ||
            Math.abs(result.sidebar - target) > 1 || Math.abs(result.draft - target) > 1 ||
            Math.abs(result.titlebarLeft - target) > 1 || Math.abs(result.workspaceLeft - target) > 1 ||
            Math.abs(result.contentDelta - (target - SIDEBAR_DEFAULT_WIDTH)) > 1)
            throw new Error("Sidebar pointer resize did not move the entire shell or persist its final width");
        for (const [key, expected] of [["Home", SIDEBAR_MIN_WIDTH], ["End", SIDEBAR_MAX_WIDTH]] as const) {
            await window.webContents.executeJavaScript(`document.querySelector('.sidebar-resize-handle').dispatchEvent(new KeyboardEvent('keydown',{key:${JSON.stringify(key)},bubbles:true}))`);
            await waitDom(window, `document.querySelector('.sidebar-resize-handle').getAttribute('aria-valuenow')==='${expected}'`, "Sidebar keyboard limit did not apply");
        }
        window.setContentSize(1000, 800);
        await waitDom(window, `window.innerWidth<=1100 && Math.abs(document.querySelector('.sidebar').getBoundingClientRect().width-${SIDEBAR_MAX_WIDTH})<1`, "A narrow-window media rule overrode the user's sidebar width");
        await window.webContents.executeJavaScript("document.querySelector('.sidebar-resize-handle').dispatchEvent(new MouseEvent('dblclick',{bubbles:true}))");
        await waitDom(window, `document.querySelector('.sidebar-resize-handle').getAttribute('aria-valuenow')==='${SIDEBAR_DEFAULT_WIDTH}'`, "Sidebar double-click did not reset its width");
        const collapse = await window.webContents.executeJavaScript(`(async()=>{
          const rail=document.querySelector('.sidebar-resize-handle');
          const sidebar=document.querySelector('.sidebar');
          const wait=()=>new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve)));
          const start=sidebar.getBoundingClientRect().right;
          const threshold=start-${SIDEBAR_DEFAULT_WIDTH}+${SIDEBAR_MIN_WIDTH - SIDEBAR_TOGGLE_DRAG_DISTANCE}-1;
          const move=x=>window.dispatchEvent(new PointerEvent('pointermove',{buttons:1,pointerId:42,clientX:x}));
          rail.dispatchEvent(new PointerEvent('pointerdown',{bubbles:true,button:0,buttons:1,pointerId:42,clientX:start}));
          move(threshold+2);await wait();const beforeThreshold=sidebar.getBoundingClientRect().width;
          move(threshold);await wait();
          const folded=sidebar.getBoundingClientRect().width;
          const left=document.querySelector('.workspace').getBoundingClientRect().left;
          const titles=[...sidebar.querySelectorAll('nav a')].every(link=>link.title && link.getAttribute('aria-label') && getComputedStyle(link.querySelector('span')).display==='none');
          const iconsFit=[...sidebar.querySelectorAll('nav a svg')].every(icon=>{const r=icon.getBoundingClientRect();return r.left>=0 && r.right<=folded});
          const boot=await window.mousebox.invoke('bootstrap',{});
          window.dispatchEvent(new PointerEvent('pointerup',{pointerId:42,clientX:threshold}));await wait();
          return {beforeThreshold,folded,left,titles,iconsFit,previewNotSaved:!boot.preferences.sidebarCollapsed};
        })()`);
        if (Math.abs(collapse.beforeThreshold - SIDEBAR_MIN_WIDTH) > 1 || Math.abs(collapse.folded - SIDEBAR_COLLAPSED_WIDTH) > 1 ||
            Math.abs(collapse.left - SIDEBAR_COLLAPSED_WIDTH) > 1 || !collapse.titles || !collapse.iconsFit || !collapse.previewNotSaved)
            throw new Error("Collapsed sidebar lost its hysteresis, shell alignment or accessible navigation: " + JSON.stringify(collapse));
        await waitDom(window, `document.querySelector('.sidebar-resize-handle').getAttribute('aria-valuenow')==='${SIDEBAR_COLLAPSED_WIDTH}'`, "Collapsed state was not committed");
        const committed = await container.invoke("bootstrap", {}) as Bootstrap;
        if (!committed.preferences.sidebarCollapsed || committed.preferences.sidebarWidth !== SIDEBAR_MIN_WIDTH)
            throw new Error("Sidebar width and collapse mode did not persist together");
        await window.webContents.executeJavaScript(`document.querySelector('.game-selector [role="combobox"]').click()`);
        await waitDom(window, `document.querySelector('.game-selector [role="combobox"]').getAttribute('aria-expanded')==='true' && !!document.querySelector('.select-popup-menu [role="option"]')`, "Collapsed game selector did not open");
        const selector = await window.webContents.executeJavaScript(`(()=>{
          const trigger=document.querySelector('.game-selector [role="combobox"]');
          return {icon:getComputedStyle(trigger.querySelector('.ui-select-leading')).display!=='none',title:!!trigger.title,
          popupWidth:document.querySelector('.select-popup-menu').getBoundingClientRect().width};})()`);
        if (!selector.icon || !selector.title || selector.popupWidth < 150) throw new Error("Collapsed game selector is not usable");
        await window.webContents.executeJavaScript(`(()=>{
          const label=document.querySelector('.game-selector .ui-select-value').textContent;
          const option=[...document.querySelectorAll('.select-popup-menu [role="option"]')].find(item=>item.textContent===label);
          if(!option)throw new Error('Current game option is missing');
          option.dispatchEvent(new PointerEvent('pointerdown',{bubbles:true,pointerType:'touch'}));option.click();
        })()`);
        // Base UI may retain a closed portal while a hidden renderer is not
        // advancing its exit animation. The trigger owns the open/closed state.
        await waitDom(window, `document.querySelector('.game-selector [role="combobox"]').getAttribute('aria-expanded')==='false'`, "Game menu failed to close");
        const expand = await window.webContents.executeJavaScript(`(async()=>{
          const rail=document.querySelector('.sidebar-resize-handle'),sidebar=document.querySelector('.sidebar');
          const start=sidebar.getBoundingClientRect().right;
          const wait=()=>new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve)));
          rail.dispatchEvent(new PointerEvent('pointerdown',{bubbles:true,button:0,buttons:1,pointerId:43,clientX:start}));
          window.dispatchEvent(new PointerEvent('pointermove',{buttons:1,pointerId:43,clientX:start+${SIDEBAR_TOGGLE_DRAG_DISTANCE - 1}}));await wait();
          const buffered=sidebar.getBoundingClientRect().width;
          window.dispatchEvent(new PointerEvent('pointermove',{buttons:1,pointerId:43,clientX:start+${SIDEBAR_TOGGLE_DRAG_DISTANCE + 1}}));await wait();
          const unfolded=sidebar.getBoundingClientRect().width;
          window.dispatchEvent(new PointerEvent('pointerup',{pointerId:43,clientX:start+${SIDEBAR_TOGGLE_DRAG_DISTANCE + 41}}));await wait();
          return {buffered,unfolded,continued:sidebar.getBoundingClientRect().width};})()`);
        if (Math.abs(expand.buffered - SIDEBAR_COLLAPSED_WIDTH) > 1 || Math.abs(expand.unfolded - SIDEBAR_MIN_WIDTH) > 1 || Math.abs(expand.continued - SIDEBAR_MIN_WIDTH - 40) > 1)
            throw new Error("Sidebar expansion jumped or lost its new drag anchor");
        await set(SIDEBAR_MIN_WIDTH, true);
        await window.webContents.executeJavaScript("document.querySelector('.sidebar-resize-handle').dispatchEvent(new MouseEvent('dblclick',{bubbles:true}))");
        await waitDom(window, `document.documentElement.dataset.sidebarCollapsed==='false' && document.querySelector('.sidebar-resize-handle').getAttribute('aria-valuenow')==='${SIDEBAR_DEFAULT_WIDTH}'`, "Collapsed double-click failed to restore the default layout");
        return { ...result, collapse, expand, compactGameSelector: selector, persistedCollapse: true, narrowWindowPreservesWidth: true, keyboardLimits: true, doubleClickReset: true, physicalInput: false, foregroundInteraction: false };
    } finally { window.setSize(originalWidth, originalHeight); await set(original, originallyCollapsed); }
}
