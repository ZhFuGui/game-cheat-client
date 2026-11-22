import { resolve } from 'node:path';
import type { Bootstrap } from '@mousebox/shared';
import type { StartupTaskContext } from './context';
export async function runStartupCalibration({ startup, native, window, container, lifecycle }: Pick<StartupTaskContext, 'startup' | 'native' | 'window' | 'container' | 'lifecycle'>): Promise<void> {
    const boot = await container.invoke('bootstrap', {}) as Bootstrap;
    if(boot.workspace?.active?.gameId && boot.workspace.active.gameId!== 'pubg')
        throw new Error('This game does not provide input calibration');
    if (boot.migration?.required)
        throw new Error('Select the settings migration source before using calibration mode');
    lifecycle.activated();
    await native.request('reader.connect', { host: boot.values.host, port: boot.values.port, hz: boot.values.hz });
    await native.request('box.connect', { port: startup.calibrationPort });
    let started = false, success = false;
    for (let attempt = 0; attempt < 200; attempt++) {
        const status = await native.request('calibration.status') as {
            boxReady: boolean;
            finished: boolean;
            succeeded: boolean;
        };
        if (!started && status.boxReady) {
            await native.request('calibration.start', { mode: boot.values.measurement });
            started = true;
        }
        else if (started && status.finished) {
            success = status.succeeded;
            if (startup.calibrationOutput)
                await native.request('calibration.export', { path: resolve(startup.calibrationOutput) });
            break;
        }
        await new Promise(resolve => setTimeout(resolve, 100));
    }
    process.exitCode = success ? 0 : 3;
    window.close();
}
