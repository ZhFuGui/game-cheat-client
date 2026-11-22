import { writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';
import type { StartupTaskContext } from './context';
export function scheduleStartupReport({ startup, native, window, version, autoConnect }: Pick<StartupTaskContext, 'startup' | 'native' | 'window' | 'version' | 'autoConnect'>): void {
    if (!startup.startupReport)
        return;
    const path = resolve(startup.startupReport);
    const timer = setTimeout(() => {
        void Promise.all([native.request('runtime.snapshot'), native.request('capture.inspect')])
            .then(([snapshot, nativeCapture]) => writeFile(path, JSON.stringify({ version, autoConnect, snapshot, nativeCapture }, null, 2)))
            .catch(error => console.error('Startup status report:', error));
    }, 5000);
    window.once('closed', () => clearTimeout(timer));
}
