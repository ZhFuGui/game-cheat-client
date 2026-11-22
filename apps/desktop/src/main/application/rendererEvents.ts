import type { BrowserWindow } from 'electron';
import { eventSchemas, workspaceTopics, type Topic, type WorkspaceContext } from '@mousebox/shared';
import { WorkspaceMessageScope } from '../domain/workspaceMessageScope';

/** Bounded display delivery; a retired workspace can never be restored from the cache. */
export function createRendererEvents(window: BrowserWindow) {
    let rendererAlive = true;
    const scope = new WorkspaceMessageScope();
    const deferred = new Map<Topic, { data: unknown; context?: WorkspaceContext }>();
    window.webContents.on('render-process-gone', () => { rendererAlive = false; });
    window.webContents.on('did-finish-load', () => { rendererAlive = true; });
    const publish = <K extends Topic>(topic: K, value: unknown, context?: WorkspaceContext) => {
        const parsed = eventSchemas[topic].safeParse(value);
        if (!parsed.success) {
            console.error(`Invalid native ${topic} event`, parsed.error.message);
            return;
        }
        if (topic === 'workspace' && scope.update(parsed.data)) {
            for (const key of deferred.keys()) if (workspaceTopics.has(key)) deferred.delete(key);
        }
        if (!scope.accepts(topic, context)) return;
        if (window.isMinimized() && ['state', 'learning', 'mouseReports', 'algorithm', 'logs'].includes(topic)) {
            if (topic === 'logs') {
                const batch = parsed.data as { entries: unknown[]; cursor: string };
                const old = deferred.get(topic)?.data as { entries: unknown[] } | undefined;
                deferred.set(topic, { data: { ...batch, entries: [...(old?.entries || []), ...batch.entries].slice(-4000) } });
            } else deferred.set(topic, { data: parsed.data, context });
            return;
        }
        if (rendererAlive && !window.isDestroyed() && !window.webContents.isDestroyed()) {
            if (context) window.webContents.send(`mousebox:event:${topic}`, parsed.data, context);
            else window.webContents.send(`mousebox:event:${topic}`, parsed.data);
        }
    };
    window.on('restore', () => {
        const pending = [...deferred];
        deferred.clear();
        for (const [topic, value] of pending) publish(topic, value.data, value.context);
    });
    window.once('closed', () => deferred.clear());
    return publish;
}
