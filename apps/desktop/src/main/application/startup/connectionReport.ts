import { writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';
import type { StartupTaskContext } from './context';
type Context = Pick<StartupTaskContext, 'startup' | 'native' | 'window' | 'version'>;
/** Optional, bounded status-only observation; never opens a device or grants output. */
export function scheduleConnectionReport({ startup, native, window, version }: Context): void {
    if (!startup.connectionReport) return;
    const path = resolve(startup.connectionReport), samples: unknown[] = [], started = Date.now();
    let busy = false, closed = false;
    const timer = setInterval(() => {
        if (closed || busy) return;
        if (samples.length >= 30) { clearInterval(timer); return; }
        busy = true;
        void native.request('runtime.snapshot').then(async snapshot => {
            if (closed) return;
            const state = (snapshot as { state: Record<string, unknown> }).state;
            samples.push({ elapsedMs: Date.now() - started,
                connections: state.connections, button: state.button,
                boxFeedbackAgeMs: state.boxFeedbackAgeMs, boxFeedbackSequence: state.boxFeedbackSequence,
                boxReceiverReady: state.boxReceiverReady, boxButtonsKnown: state.boxButtonsKnown,
                deviceBaudRate: state.deviceBaudRate, sessionEligible: state.sessionEligible });
            await writeFile(path, JSON.stringify({ version, samples }, null, 2));
        }).catch(error => { clearInterval(timer); console.error('Connection status report:', error); })
            .finally(() => { busy = false; });
    }, 1000);
    const deadline = setTimeout(() => clearInterval(timer), 35000);
    window.once('closed', () => { closed = true; clearInterval(timer); clearTimeout(deadline); });
}
