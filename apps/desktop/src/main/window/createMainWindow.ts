import { app, BrowserWindow, nativeTheme, shell } from 'electron';
import { join } from 'node:path';
import type { UiPreferenceStore } from '../infrastructure/storage/UiPreferences';
import { isTrustedUrl } from '../ipc/validateSender';
import type { StartupOptions } from '../application/startup';
export type MainWindowOptions = {
    uiPath: string;
    preload: string;
    devOrigin?: string;
    hidden: boolean;
    startup: Pick<StartupOptions, 'route' | 'algorithmOpen' | 'minimized'>;
};
export function createMainWindow(store: UiPreferenceStore, options: MainWindowOptions): BrowserWindow {
    const { uiPath, preload, devOrigin, hidden, startup: start } = options;
    const prefs = store.get();
    nativeTheme.themeSource = prefs.colorMode;
    const window = new BrowserWindow({ width: prefs.width, height: prefs.height, minWidth: 960, minHeight: 640,
        frame: false, show: false, title: app.getName(), icon: app.isPackaged ? join(process.resourcesPath, 'icon.ico') : join(app.getAppPath(), 'resources', 'icon.ico'), backgroundColor: nativeTheme.shouldUseDarkColors ? '#171717' : '#f4f6fa',
        webPreferences: { preload, contextIsolation: true, nodeIntegration: false, sandbox: true, spellcheck: false, webSecurity: true, backgroundThrottling: false } });
    window.webContents.setWindowOpenHandler(({ url }) => { if (url.startsWith('https://'))
        void shell.openExternal(url); return { action: 'deny' }; });
    window.webContents.on('will-navigate', (event, url) => { if (!isTrustedUrl(url, uiPath, devOrigin))
        event.preventDefault(); });
    window.webContents.session.setPermissionRequestHandler((_contents, _permission, callback) => callback(false));
    window.on('resize', () => { if (!window.isMaximized() && !window.isMinimized()) {
        const [width, height] = window.getSize();
        store.set({ width, height });
    } });
    if (!hidden)
        window.once('ready-to-show', () => { if (start.minimized)
            window.showInactive();
        else
            window.show(); if (start.minimized)
            window.minimize(); });
    const route = new URL(start.route, 'https://mousebox.local');
    const query: Record<string, string> = { page: route.pathname.slice(1) || 'overview' };
    const section = route.searchParams.get('section');
    if (section)
        query.section = section;
    const view = route.searchParams.get('view');
    if (view)
        query.view = view;
    if (start.algorithmOpen)
        query.tab = 'algorithm';
    if (devOrigin) {
        const url = new URL(devOrigin);
        for (const [key, value] of Object.entries(query))
            url.searchParams.set(key, value);
        void window.loadURL(url.toString());
    }
    else
        void window.loadFile(uiPath, { query });
    return window;
}
