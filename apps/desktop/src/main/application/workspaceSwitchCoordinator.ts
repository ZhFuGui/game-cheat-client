import { sameWorkspaceContext, type WorkspaceContext } from '@mousebox/shared';
import type { NativePort } from '../domain/ports';
import type { AutomaticConnections } from './automaticConnections';
import type { DesktopSnapshots } from './desktopSnapshots';
import type { SurfaceCoordinator } from './surfaceCoordinator';

interface WorkspaceSwitchDependencies {
    native: NativePort;
    automatic: Partial<Pick<AutomaticConnections, 'invalidateSource' | 'reset'>>;
    surfaces: Partial<Pick<SurfaceCoordinator, 'reset'>>;
    snapshots: Pick<DesktopSnapshots, 'refreshWorkspace'>;
    autoConnect: boolean;
    onActivated?: () => void;
}

/** Serializes workspace commits and keeps output revoked when any stage fails. */
export class WorkspaceSwitchCoordinator {
    private switching = false;
    private recoveryRequired = false;
    private operations = Promise.resolve();
    constructor(private readonly dependencies: WorkspaceSwitchDependencies) { }

    assertCommandContext(context?: WorkspaceContext): void {
        this.assertIdle();
        const { native } = this.dependencies;
        if (context && native.workspaceContext && !sameWorkspaceContext(context, native.workspaceContext()))
            throw new Error('Workspace context changed');
    }

    bootstrap<T>(operation: (activationAllowed: boolean) => Promise<T>): Promise<T> {
        return this.enqueue(() => operation(!this.recoveryRequired));
    }

    switch(payload: Record<string, unknown>): Promise<unknown> {
        this.assertIdle();
        this.switching = true;
        this.recoveryRequired = true;
        return this.enqueue(async () => {
            const { native, automatic, surfaces, snapshots, autoConnect, onActivated } = this.dependencies;
            let activationAttempted = false;
            try {
                automatic.invalidateSource?.();
                await native.revoke();
                await surfaces.reset?.();
                const catalog = await native.request('workspace.switch', payload, 45000);
                const snapshot = await snapshots.refreshWorkspace();
                activationAttempted = true;
                await native.activate();
                if (autoConnect) automatic.reset?.(snapshot);
                onActivated?.();
                this.recoveryRequired = false;
                return catalog;
            } catch (error) {
                // A timed-out activation can already have reached native. Revoke it
                // before refreshing the committed identity or admitting another bootstrap.
                if (activationAttempted) await native.revoke().catch(() => undefined);
                await snapshots.refreshWorkspace().catch(() => undefined);
                throw error;
            } finally {
                this.switching = false;
            }
        });
    }

    private assertIdle(): void {
        if (this.switching) throw new Error('Workspace switch is in progress');
    }

    private enqueue<T>(operation: () => Promise<T>): Promise<T> {
        const result = this.operations.then(operation);
        this.operations = result.then(() => undefined, () => undefined);
        return result;
    }
}
