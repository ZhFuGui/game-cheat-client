import { app, dialog, type BrowserWindow } from 'electron';
import { writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';
import { desktopLaunchOptions } from './application/launchOptions';
import { launchDesktop } from './application/launchDesktop';
import { defaultDataDirectory } from './infrastructure/migration/DataDirectoryMigration';
const options = desktopLaunchOptions(process.argv);
app.setPath('userData', options.dataDirectory || defaultDataDirectory(process.env.LOCALAPPDATA || app.getPath('appData')));
if (!app.requestSingleInstanceLock())
    app.quit();
else {
    let primary: BrowserWindow | undefined, openRequested = false;
    app.on('second-instance', () => {
        openRequested = true;
        if (primary && !primary.isDestroyed()) {
            if (primary.isMinimized())
                primary.restore();
            primary.show();
            primary.focus();
        }
    });
    app.on('window-all-closed', () => app.quit());
    void app.whenReady().then(() => launchDesktop(options, window => {
        primary = window;
        window.once('ready-to-show', () => {
            if (openRequested) {
                if (window.isMinimized())
                    window.restore();
                window.show();
                window.focus();
            }
        });
    })).catch(async (error: unknown) => {
        console.error(error);
        if (options.smokeReport) {
            await writeFile(resolve(options.smokeReport), JSON.stringify({ ok: false, error: String(error) }));
            app.exit(1);
        }
        else {
            dialog.showErrorBox('mousebox-aimbot', error instanceof Error ? error.message : String(error));
            app.quit();
        }
    });
}
