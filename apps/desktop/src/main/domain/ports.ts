import type { Topic, UiPreferences as RendererPreferences, WorkspaceContext } from '@mousebox/shared';
export interface NativePort {
    request(command: string, payload?: unknown, timeout?: number): Promise<unknown>;
    activate(): Promise<void>;
    revoke(): Promise<void>;
    workspaceContext?(): WorkspaceContext | null | undefined;
}
export interface NativeLifecyclePort {
    renew(): Promise<void>;
    revoke(): Promise<void>;
    stop(): Promise<void>;
}
export type UiPreferences = RendererPreferences & {
    width: number;
    height: number;
};
export interface UiPreferencePort {
    get(): UiPreferences;
    set(patch: Partial<UiPreferences>): UiPreferences;
}
export type LegacySource = { id: string; path: string; modifiedAt: string; hash: string };
export type RendererPublisher = <K extends Topic>(topic: K, value: unknown, context?: WorkspaceContext) => void;
export interface FileCommandsPort {
    open(purpose: string, multiple?: boolean): Promise<{ paths: string[] }>;
    save(purpose: string): Promise<{ path: string | null }>;
    validate(command: string, payload: Record<string, unknown>): void;
}
