import { bootstrapSchema, type Bootstrap } from '@mousebox/shared';
import type { NativePort, RendererPublisher, UiPreferencePort } from '../domain/ports';

interface DesktopSnapshotDependencies {
    native: Pick<NativePort, 'request'>;
    preferences: Pick<UiPreferencePort, 'get'>;
    publish: RendererPublisher;
    version: string;
    session: string;
}

/** One desktop session owns bootstrap metadata and the ordered publication of a new workspace. */
export class DesktopSnapshots {
    constructor(private readonly dependencies: DesktopSnapshotDependencies) { }

    async read(migration: NonNullable<Bootstrap['migration']>): Promise<Bootstrap> {
        const { native, preferences, version, session } = this.dependencies;
        const snapshot = await native.request('bootstrap');
        const { language, colorMode, sidebarWidth,sidebarCollapsed,pageTabOrder,espPlacement,espCapture,espRenderer } = preferences.get();
        return bootstrapSchema.parse({ ...snapshot as object, preferences: { language, colorMode, sidebarWidth,sidebarCollapsed,pageTabOrder,espPlacement,espCapture,espRenderer },
            version, session, migration });
    }

    publishIdentity(snapshot: Bootstrap): void {
        const { publish } = this.dependencies;
        if (snapshot.workspace) publish('workspace', snapshot.workspace);
        if (snapshot.application) publish('application', snapshot.application);
        if (snapshot.devices) publish('devices', snapshot.devices);
    }

    async refreshWorkspace(): Promise<Bootstrap> {
        const snapshot = await this.read({ required: false, sources: [] });
        this.publishIdentity(snapshot);
        const context = snapshot.workspace?.active ?? undefined;
        for (const topic of ['values', 'espValues', 'state', 'learning', 'mouseReports', 'ports', 'speedPresets', 'graphicsAdapters'] as const)
            this.dependencies.publish(topic, snapshot[topic], context);
        return snapshot;
    }
}
