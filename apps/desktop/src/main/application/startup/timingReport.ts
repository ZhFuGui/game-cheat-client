import { mkdir, writeFile, rename } from 'node:fs/promises';
import { dirname, resolve } from 'node:path';
import type { StartupTaskContext } from './context';
import { timingReportSample } from './timingReportData';

type Context = Pick<StartupTaskContext, 'startup' | 'native' | 'window' | 'version'>;

/** Opt-in, bounded observation through the existing owner of native stdio. */
export function scheduleTimingReport({ startup, native, window, version }: Context): void {
    if (!startup.timingReport) return;
    const path = resolve(startup.timingReport), started = Date.now();
    const samples: unknown[] = [];
    let busy = false, closed = false, finished = false, persisted = false, writing = false, errors = 0, stopReason = 'sampling';
    const save = async () => {
        if (writing || persisted) return;
        writing = true;
        const complete = finished;
        try {
            await mkdir(dirname(path), { recursive: true });
            const temporary = `${path}.${process.pid}.tmp`;
            await writeFile(temporary, JSON.stringify({ schemaVersion: 1, version, desktopPid: process.pid,
                collectionScope: 'post_startup_instance', oldProcessSampled: false, complete,
                startedAtUtc: new Date(started).toISOString(), savedAtUtc: new Date().toISOString(),
                requestedDurationMs: 20000, intervalMs: 1000, stopReason, errors, samples }, null, 2));
            await rename(temporary, path);
            persisted = complete;
        } finally { writing = false; }
    };
    const flush = () => { if (!busy) void save().catch(() => console.error('Read-only timing report could not be saved')); };
    const finish = (reason: string) => {
        if (finished) return;
        finished = true;
        stopReason = reason;
        clearInterval(timer);
        clearTimeout(deadline);
        flush();
    };
    const observe = async () => {
        if (busy || finished || samples.length >= 20) return;
        busy = true;
        try {
            const began = Date.now();
            const algorithm = await native.request('algorithm.read', {}, 1500);
            if (closed) return;
            const runtime = await native.request('runtime.snapshot', {}, 1500);
            if (closed) return;
            samples.push({ elapsedMs: began - started, completedElapsedMs: Date.now() - started,
                ...timingReportSample(algorithm, runtime) });
            await save();
        } catch {
            // Never persist arbitrary errors, which can contain paths or payloads.
            if (!closed) ++errors;
        } finally {
            busy = false;
            if (finished) flush();
        }
    };
    const timer = setInterval(() => { void observe(); }, 1000);
    const deadline = setTimeout(() => finish('duration_complete'), 20000);
    window.once('closed', () => { closed = true; finish('window_closed'); });
    void observe();
}
