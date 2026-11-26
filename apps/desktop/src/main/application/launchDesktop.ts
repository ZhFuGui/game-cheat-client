import { app, session, Menu, type BrowserWindow } from 'electron';
import { join, resolve } from 'node:path';
import { mkdir, readFile, writeFile } from 'node:fs/promises';
import { createContainer, type DesktopContainer } from './container';
import { prepareRuntime } from './prepareRuntime';
import { createMainWindow } from '../window/createMainWindow';
import { installContentSecurityPolicy } from '../window/contentSecurity';
import { registerHandlers } from '../ipc/registerHandlers';
import { legacyOperationalSettings, legacyUiPreferences } from '../infrastructure/migration/LegacySettingsImporter';
import { runStartupTasks } from './startupTasks';
import type { NativePort } from '../domain/ports';
import { ForegroundInput } from '../infrastructure/native/ForegroundInput';
import type { DesktopLaunchOptions } from './launchOptions';

type SmokeRunner = (context: {
    window: BrowserWindow;
    native: NativePort;
    container: DesktopContainer;
    report: string;
    version: string;
    args: string[];
}) => Promise<void>;
const smokeRunners = import.meta.glob<SmokeRunner>('../verification/runSmoke.ts', { eager: true, import: 'runSmoke' });

/** Production composition root; optional verification never changes the normal startup path. */
export async function launchDesktop(options: DesktopLaunchOptions, onWindow: (window: BrowserWindow) => void): Promise<void> {
    const { startup, smokeReport, autoConnect } = options;
    const runSmoke = smokeRunners['../verification/runSmoke.ts'];
    if (smokeReport && !options.explicitlyIsolated)
        throw new Error('Smoke verification requires an isolated --data-dir');
    if (smokeReport && !runSmoke)
        throw new Error('Smoke verification requires the local desktop verification files at build time');
    Menu.setApplicationMenu(null);
    const repo = resolve(__dirname, '../../../..');
    const uiPath = app.isPackaged ? join(process.resourcesPath, 'ui', 'index.html') : join(repo, 'apps', 'ui', 'dist', 'index.html');
    const executable = app.isPackaged ? join(process.resourcesPath, 'native', 'mousebox-native.exe') : join(repo, 'out', 'x64', 'Release', 'mousebox-native.exe');
    const dataDirectory = app.getPath('userData');
    const prepared = await prepareRuntime(dataDirectory, executable, options.migrateLegacy);
    const input = new ForegroundInput(executable);
    prepared.native.setCaptureToken(input.token);
    prepared.native.once('failure', () => input.close());
    app.once('will-quit', () => input.close());
    if (startup.settings) {
        const ini = await readFile(resolve(startup.settings), 'utf8');
        await mkdir(dataDirectory, { recursive: true });
        const file = join(dataDirectory, 'legacy-import.json');
        await writeFile(file, JSON.stringify(legacyOperationalSettings(ini)));
        prepared.native.setInitialSettings(file);
        prepared.preferences.set(legacyUiPreferences(ini));
    }
    const devOrigin = process.env.ELECTRON_RENDERER_URL;
    installContentSecurityPolicy(session.defaultSession, devOrigin);
    const window = createMainWindow(prepared.preferences, { uiPath, preload: join(__dirname, '../preload/index.cjs'), devOrigin, startup, hidden: Boolean(smokeReport) });
    onWindow(window);
    window.once('closed', () => input.close());
    const container = createContainer(window, prepared, { autoConnect,allowOverlay:!smokeReport&&!startup.screenshot&&!startup.renderReport&&!startup.evidence });
    const { lifecycle } = container;
    const dispose = registerHandlers(window, uiPath, devOrigin, container.invoke, lifecycle.pulse);
    window.once('closed', dispose);
    prepared.native.start();
    await runStartupTasks({ startup, window, native: prepared.native, container, lifecycle, dataDirectory,
        version: app.getVersion(), autoConnect });
    if (smokeReport && runSmoke) {
        window.webContents.once('did-finish-load', () => {
            void runSmoke({ window, native: prepared.native,
                container, report: smokeReport, version: app.getVersion(), args: options.args });
        });
    }
}
