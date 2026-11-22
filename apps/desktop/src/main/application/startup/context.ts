import type { BrowserWindow } from 'electron';
import type { DesktopContainer } from '../container';
import type { NativePort } from '../../domain/ports';
import type { StartupOptions } from '../startup';
export interface StartupTaskContext {
    startup: StartupOptions;
    window: BrowserWindow;
    native: NativePort;
    container: DesktopContainer;
    lifecycle: {
        activated(): void;
    };
    dataDirectory: string;
    version: string;
    autoConnect: boolean;
}
