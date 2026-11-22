import { app, clipboard, nativeTheme, type BrowserWindow } from 'electron';
import { randomUUID } from 'node:crypto';
import { eventSchemas, type Topic, type WorkspaceContext } from '@mousebox/shared';
import { NativeSurfaces } from '../window/nativeSurface';
import { FileUseCases } from './fileUseCases';
import { AutomaticConnections } from './automaticConnections';
import { createRendererEvents } from './rendererEvents';
import { createCommandRouter } from './commandRouter';
import { SurfaceCoordinator } from './surfaceCoordinator';
import type { PreparedRuntime } from './prepareRuntime';
import { manageLifecycle } from '../window/lifecycle';
export function createContainer(window: BrowserWindow, prepared: PreparedRuntime, options: {
    autoConnect?: boolean;
    allowOverlay?: boolean;
} = {}) {
    const { native } = prepared;
    const surface = new NativeSurfaces(window, native,options.allowOverlay??true);
    const surfaces = new SurfaceCoordinator(surface);
    const publish = createRendererEvents(window);
    const automatic = new AutomaticConnections(native, error => publish('error', { code: 'auto_connect', message: error.message }));
    window.once('close', () => automatic.dispose());
    window.webContents.on('render-process-gone', () => automatic.dispose());
    native.on('event', (topic: string, data: unknown, context?: WorkspaceContext) => {
        if (topic === 'ports') {
            const value = eventSchemas.ports.safeParse(data);
            if (value.success)
                automatic.updatePorts(value.data);
        }
        if (topic === 'state') {
            const value = eventSchemas.state.safeParse(data);
            if (value.success)
                automatic.updateState(value.data);
        }
        if (topic === 'devices') {
            const value = eventSchemas.devices.safeParse(data);
            if (value.success)
                automatic.updateDevices(value.data);
        }
        if (Object.prototype.hasOwnProperty.call(eventSchemas, topic))
            publish(topic as Topic, data, context);
    });
    native.on('failure', (error: Error) => { automatic.dispose(); publish('bridge', { connected: false, message: error.message }); });
    const files = new FileUseCases(window, value => surfaces.setOccluded(value));
    const lifecycle = manageLifecycle(window, native, () => surface.minimize());
    const invoke = createCommandRouter({ ...prepared, window, files, automatic, surfaces, publish,
        version: app.getVersion(), session: randomUUID(), autoConnect: Boolean(options.autoConnect),
        setColorMode: value => { nativeTheme.themeSource = value; },
        writeClipboard: text => clipboard.writeText(text), onActivated: lifecycle.activated });
    return { invoke, surface, publish, lifecycle };
}
export type DesktopContainer = ReturnType<typeof createContainer>;
