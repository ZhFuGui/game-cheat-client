import { contracts, preferencesSchema, isWorkspaceCommand, type CommandName, type WorkspaceContext } from '@mousebox/shared';
import type { FileCommandsPort, LegacySource, NativePort, RendererPublisher, UiPreferencePort } from '../domain/ports';
import type { AutomaticConnections } from './automaticConnections';
import type { SurfaceCoordinator } from './surfaceCoordinator';
import { DesktopSnapshots } from './desktopSnapshots';
import { WorkspaceSwitchCoordinator } from './workspaceSwitchCoordinator';
export type CommandInvoker = (name: CommandName, args: unknown, context?: WorkspaceContext) => Promise<unknown>;
export interface CommandRouterDependencies {
    native: NativePort;
    preferences: UiPreferencePort;
    sources: LegacySource[];
    migrationRequired: boolean;
    importSource: (source: LegacySource) => Promise<unknown>;
    window: { close(): void; minimize(): void; isMaximized(): boolean; maximize(): void; unmaximize(): void };
    files: FileCommandsPort;
    automatic: Pick<AutomaticConnections, 'start' | 'manual'> & Partial<Pick<AutomaticConnections, 'reset' | 'invalidateSource'>>;
    surfaces: Pick<SurfaceCoordinator, 'update'> & Partial<Pick<SurfaceCoordinator, 'reset'|'configure'>>;
    publish: RendererPublisher;
    version: string;
    session: string;
    autoConnect: boolean;
    setColorMode: (value: 'light' | 'dark') => void;
    writeClipboard: (text: string) => void;
    onActivated?: () => void;
}
/** Desktop commands are handled locally; business commands retain native validation. */
export function createCommandRouter(dependencies: CommandRouterDependencies): CommandInvoker {
    const { native, preferences, sources, window, files, automatic, surfaces, publish, autoConnect, setColorMode, writeClipboard } = dependencies;
    let migrationRequired = dependencies.migrationRequired;
    const snapshots = new DesktopSnapshots(dependencies);
    const workspaces = new WorkspaceSwitchCoordinator({ native, automatic, surfaces, snapshots, autoConnect,
        onActivated: dependencies.onActivated });
    return async function invoke(name: CommandName, args: unknown, context?: WorkspaceContext): Promise<unknown> {
        const payload = contracts[name].input.parse(args) as Record<string, unknown>;
        if (isWorkspaceCommand(name, payload)) workspaces.assertCommandContext(context);
        if (name === 'bootstrap') {
            return workspaces.bootstrap(async activationAllowed => {
                const activate = activationAllowed && !migrationRequired;
                if (activate) await native.activate();
                await surfaces.configure?.(preferences.get());
                const result = await snapshots.read({ required: migrationRequired,
                    sources: sources.map(({ id, path, modifiedAt }) => ({ id, path, modifiedAt })) });
                snapshots.publishIdentity(result);
                if (autoConnect && activate) automatic.start(result);
                if (activate) dependencies.onActivated?.();
                return result;
            });
        }
        if (name === 'preferences.set') {
            const before=preferences.get();
            // Each renderer update owns one group; concurrent groups must not replace one another.
            const patch=payload.pageTabOrder?{...payload,pageTabOrder:{...before.pageTabOrder,...payload.pageTabOrder as Record<string,string[]>}}:payload;
            const next={...before,...patch};
            const outputChanged=payload.espPlacement!==undefined||payload.espCapture!==undefined||payload.espRenderer!==undefined;
            if(outputChanged) await surfaces.configure?.(next);
            let value;
            try{value=preferences.set(patch);}
            catch(error){if(outputChanged)await surfaces.configure?.(before);throw error;}
            if (payload.colorMode !== undefined) setColorMode(value.colorMode);
            const parsed = preferencesSchema.parse({ language: value.language, colorMode: value.colorMode, themePreset: value.themePreset,
                sidebarWidth: value.sidebarWidth,sidebarCollapsed:value.sidebarCollapsed,pageTabOrder:value.pageTabOrder,espPlacement:value.espPlacement,espCapture:value.espCapture,espRenderer:value.espRenderer });
            publish('preferences', parsed);
            return parsed;
        }
        if (name === 'migration.choose') {
            if (!migrationRequired)
                throw new Error('Migration is already complete');
            const source = sources.find(s => s.id === payload.id);
            if (!source)
                throw new Error('Unknown migration source');
            const settings = await dependencies.importSource(source);
            await native.request('settings.import', settings);
            migrationRequired = false;
            await native.activate();
            dependencies.onActivated?.();
            return { ok: true };
        }
        if (name === 'window.close') {
            window.close();
            return { ok: true };
        }
        if (name === 'window.minimize') {
            window.minimize();
            return { ok: true };
        }
        if (name === 'window.maximize') {
            if (window.isMaximized())
                window.unmaximize();
            else
                window.maximize();
            return { ok: true };
        }
        if (name === 'surface.update')
            return surfaces.update(payload);
        if (name === 'file.open')
            return files.open(payload.purpose as string, payload.multiple as boolean | undefined);
        if (name === 'file.save')
            return files.save(payload.purpose as string);
        if (name === 'clipboard.write') {
            writeClipboard(payload.text as string);
            return {ok:true};
        }
        if (name === 'algorithm.copy') {
            const result = await native.request('algorithm.read');
            writeClipboard(JSON.stringify(result, null, 2));
            return { ok: true };
        }
        if (migrationRequired)
            throw new Error('请先选择需要迁移的配置来源');
        if (name === 'workspace.switch') return workspaces.switch(payload);
        files.validate(name, payload);
        automatic.manual(name);
        return native.request(name, payload, name === 'samples.import' || name === 'recording.load' ? 60000 : 15000);
    };
}
