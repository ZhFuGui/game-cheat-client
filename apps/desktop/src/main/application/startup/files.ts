import { writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';
import type { NativePort } from '../../domain/ports';
import type { StartupTaskContext } from './context';
/** Imports are sequential: each native background operation must finish before the next. */
export async function waitForNativeIo(native: Pick<NativePort, 'request'>): Promise<void> {
    for (let attempt = 0; attempt < 600; attempt++) {
        const state = await native.request('runtime.snapshot') as {
            learning: {
                ioBusy?: boolean;
                feedback?: string;
            };
        };
        if (!state.learning.ioBusy) {
            if (state.learning.feedback?.startsWith('文件操作失败'))
                throw new Error(state.learning.feedback);
            return;
        }
        await new Promise(resolve => setTimeout(resolve, 100));
    }
    throw new Error('Startup file operation timed out');
}
export async function importStartupFiles({ startup, native }: Pick<StartupTaskContext, 'startup' | 'native'>): Promise<void> {
    if (startup.trace)
        await writeFile(resolve(startup.trace), 'Electron desktop started\n');
    if (startup.evidence || startup.samples.length || startup.model)
        await native.request('bootstrap');
    if (startup.evidence) {
        await native.request('recording.load', { path: resolve(startup.evidence) });
        await waitForNativeIo(native);
    }
    if (startup.samples.length) {
        await native.request('samples.import', { paths: startup.samples.map(path => resolve(path)) });
        await waitForNativeIo(native);
    }
    if (startup.model) {
        await native.request('model.import', { path: resolve(startup.model) });
        await waitForNativeIo(native);
    }
}
