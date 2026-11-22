import type { BrowserWindow } from 'electron';
import type { DesktopContainer } from '../application/container';
import type { NativePort } from '../domain/ports';
export interface SmokeContext {
    window: BrowserWindow;
    native: NativePort;
    container: DesktopContainer;
    report: string;
    version: string;
    args: string[];
}
